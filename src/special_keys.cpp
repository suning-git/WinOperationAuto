#include "special_keys.h"
#include "input_injection.h"
#include "suggestion_overlay.h"
#include <iostream>
#include <fstream>

// Static member definitions
std::vector<USHORT> SpecialKeyHandler::s_specialKeys;
std::vector<SpecialKeyState> SpecialKeyHandler::s_specialKeyStates;
std::string SpecialKeyHandler::s_pendingSuggestion = "";
bool SpecialKeyHandler::s_hasPendingSuggestion = false;

void SpecialKeyHandler::Initialize() {
    // Initialize the list of special keys we want to monitor
    // Note: We use VK_CONTROL for both left/right Ctrl and distinguish by flags
    s_specialKeys = {
        VK_CONTROL,     // Ctrl (0x11) - we'll distinguish left/right by flags
        VK_SHIFT,       // Shift (0x10)
        VK_MENU         // Alt (0x12)
    };
    
    // Initialize state tracking for each special key
    s_specialKeyStates.resize(s_specialKeys.size());
    
    std::cout << "[OK] Special key handler initialized\n";
}

bool SpecialKeyHandler::IsSpecialKey(USHORT vKey) {
    return GetSpecialKeyIndex(vKey) >= 0;
}

int SpecialKeyHandler::GetSpecialKeyIndex(USHORT vKey) {
    for (size_t i = 0; i < s_specialKeys.size(); ++i) {
        if (s_specialKeys[i] == vKey) {
            return static_cast<int>(i);
        }
    }
    return -1; // Not a special key
}

const char* SpecialKeyHandler::GetKeyName(USHORT vKey) {
    switch (vKey) {
        case VK_CONTROL: return "Ctrl";
        case VK_SHIFT: return "Shift";
        case VK_MENU: return "Alt";
        default: return "Unknown";
    }
}

bool SpecialKeyHandler::ProcessSpecialKeyEvent(USHORT vKey, USHORT flags, bool isKeyUp, std::uint64_t timestamp, POINT cursorPos) {
    int specialKeyIndex = GetSpecialKeyIndex(vKey);
    if (specialKeyIndex < 0) {
        return false; // Not a special key
    }
    
    SpecialKeyState& keyState = s_specialKeyStates[specialKeyIndex];
    
    if (!isKeyUp) {
        // Key pressed down
        keyState.isPressed = true;
        keyState.hadInterveningKeys = false;  // Reset the flag
        keyState.pressTimestamp = timestamp;
        keyState.pressPosition = cursorPos;
    } else {
        // Key released
        if (keyState.isPressed && !keyState.hadInterveningKeys) {
            // Complete press cycle detected WITHOUT intervening keys - trigger the handler
            // For Ctrl keys, we need to distinguish left vs right using flags
            if (vKey == VK_CONTROL) {
                bool isRightCtrl = (flags & RI_KEY_E0) != 0;
                if (isRightCtrl) {
                    std::cout << "[SPECIAL] Right Ctrl pressed alone (no key combinations)\n";
                    OnRightCtrlPressed(keyState.pressTimestamp, timestamp, keyState.pressPosition, cursorPos);
                } else {
                    std::cout << "[TRIGGER] Left Ctrl pressed - triggering input completion\n";
                    OnLeftCtrlPressed(keyState.pressTimestamp, timestamp, keyState.pressPosition, cursorPos);
                }
            } else {
                std::cout << "[SPECIAL] " << GetKeyName(vKey) << " pressed alone (no key combinations)\n";
                OnSpecialKeyPressed(vKey, keyState.pressTimestamp, timestamp, 
                                  keyState.pressPosition, cursorPos);
            }
        } else if (keyState.isPressed && keyState.hadInterveningKeys) {
            // Special key was part of a combination - don't trigger handler
            if (vKey == VK_CONTROL) {
                bool isRightCtrl = (flags & RI_KEY_E0) != 0;
                std::cout << "[SPECIAL] " << (isRightCtrl ? "Right Ctrl" : "Left Ctrl") << " was part of key combination - handler not triggered\n";
            } else {
                std::cout << "[SPECIAL] " << GetKeyName(vKey) << " was part of key combination - handler not triggered\n";
            }
        }
        keyState.isPressed = false;
        keyState.hadInterveningKeys = false;
    }
    
    return true; // Event was handled
}

void SpecialKeyHandler::AddSpecialKey(USHORT vKey) {
    // Check if key is already being monitored
    if (GetSpecialKeyIndex(vKey) >= 0) {
        return; // Already exists
    }
    
    s_specialKeys.push_back(vKey);
    s_specialKeyStates.push_back(SpecialKeyState());
}

const std::vector<USHORT>& SpecialKeyHandler::GetSpecialKeys() {
    return s_specialKeys;
}

void SpecialKeyHandler::NotifyRegularKeyPressed(USHORT vKey) {
    // Check if any special keys are currently pressed
    for (size_t i = 0; i < s_specialKeys.size(); ++i) {
        if (s_specialKeyStates[i].isPressed) {
            // Mark this special key as having intervening keys
            s_specialKeyStates[i].hadInterveningKeys = true;
            std::cout << "[COMBO] " << GetKeyName(s_specialKeys[i]) << " + " << std::hex << "0x" << vKey << std::dec << " detected\n";
        }
    }
}

// Specific handler for Left Ctrl key press - Generate suggestion
void SpecialKeyHandler::OnLeftCtrlPressed(std::uint64_t pressTime, std::uint64_t releaseTime, POINT pressPos, POINT releasePos) {
    std::uint64_t duration = releaseTime - pressTime;
    
    std::cout << "\n[AI] Generating input completion...\n";
    
    // Call Python script to process input_events.txt (script is now in same directory as exe)
    std::string pythonCmd = "python process_input.py";
    int result = system(pythonCmd.c_str());
    
    if (result == 0) {
        
        // Read Python output (now in same directory as executable)
        std::ifstream outputFile("python_output.txt");
        if (outputFile.is_open()) {
            std::string outputLine;
            if (std::getline(outputFile, outputLine)) {
                outputFile.close();
                
                if (!outputLine.empty()) {
                    // Store suggestion for potential acceptance with Right Ctrl
                    s_pendingSuggestion = outputLine;
                    s_hasPendingSuggestion = true;
                    
                    // Show suggestion in overlay
                    SuggestionOverlay::ShowSuggestion(outputLine);
                    
                    std::cout << "\n[READY] Completion: \"" << outputLine << "\"\n";
                    std::cout << "[READY] Press RIGHT CTRL to accept, or ignore to cancel\n";
                    
                } else {
                    std::cout << "[INFO] No completion available\n";
                    s_hasPendingSuggestion = false;
                }
            } else {
                std::cout << "[ERROR] Could not read Python output\n";
                s_hasPendingSuggestion = false;
            }
        } else {
            std::cout << "[ERROR] Could not open python_output.txt\n";
            s_hasPendingSuggestion = false;
        }
    } else {
        std::cout << "[ERROR] Python script failed with exit code: " << result << "\n";
        s_hasPendingSuggestion = false;
    }
    
    std::cout << "*** Suggestion generation completed ***\n\n";
}

// Specific handler for Right Ctrl key press - Accept suggestion
void SpecialKeyHandler::OnRightCtrlPressed(std::uint64_t pressTime, std::uint64_t releaseTime, POINT pressPos, POINT releasePos) {
    std::uint64_t duration = releaseTime - pressTime;
    
    std::cout << "\n*** RIGHT CTRL PRESSED ***\n";
    
    if (s_hasPendingSuggestion && !s_pendingSuggestion.empty()) {
        std::cout << " ACCEPTING LLM SUGGESTION: \"" << s_pendingSuggestion << "\"\n";
        
        // Inject the LLM response as text using InputInjector
        bool success = InputInjector::SendTextString(s_pendingSuggestion);
        
        if (success) {
            std::cout << "[SUCCESS] Injected LLM text: " << s_pendingSuggestion.length() << " characters\n";
        } else {
            std::cout << "[ERROR] Failed to inject LLM text\n";
        }
        
        // Clear pending suggestion and hide overlay
        s_pendingSuggestion = "";
        s_hasPendingSuggestion = false;
        SuggestionOverlay::HideSuggestion();
        
    } else {
        std::cout << " NO PENDING SUGGESTION TO ACCEPT\n";
        std::cout << "   Press LEFT CTRL first to generate a suggestion\n";
    }
    
    std::cout << "*** Right Ctrl processing completed ***\n\n";
}

// Specific handler for Shift key press
void SpecialKeyHandler::OnShiftPressed(std::uint64_t pressTime, std::uint64_t releaseTime, POINT pressPos, POINT releasePos) {
    std::uint64_t duration = releaseTime - pressTime;
    
    std::cout << "\n*** SHIFT KEY PRESSED ***\n";
}

// Specific handler for Alt key press
void SpecialKeyHandler::OnAltPressed(std::uint64_t pressTime, std::uint64_t releaseTime, POINT pressPos, POINT releasePos) {
    std::uint64_t duration = releaseTime - pressTime;
    
    std::cout << "\n*** ALT KEY PRESSED ***\n";
}

// Generic handler that routes to specific key handlers
void SpecialKeyHandler::OnSpecialKeyPressed(USHORT vKey, std::uint64_t pressTime, std::uint64_t releaseTime, POINT pressPos, POINT releasePos) {
    // Route to specific key handlers (Ctrl is handled separately in ProcessSpecialKeyEvent)
    switch (vKey) {
        case VK_SHIFT:
            OnShiftPressed(pressTime, releaseTime, pressPos, releasePos);
            break;
        case VK_MENU:
            OnAltPressed(pressTime, releaseTime, pressPos, releasePos);
            break;
        default:
            // Fallback generic handler
            std::cout << "\n*** SPECIAL KEY HOOK TRIGGERED ***\n";
            std::cout << "Key: " << GetKeyName(vKey) << " (VK=0x" << std::hex << vKey << std::dec << ")\n";
            std::cout << "Press duration: " << (releaseTime - pressTime) << " microseconds\n";
            std::cout << "Press position: (" << pressPos.x << ", " << pressPos.y << ")\n";
            std::cout << "Release position: (" << releasePos.x << ", " << releasePos.y << ")\n";
            std::cout << "********************************\n\n";
            break;
    }
}