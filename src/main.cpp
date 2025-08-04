#include <windows.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstdint>
#include <vector>
#include "special_keys.h"
#include "input_injection.h"
#include "event_logger.h"
#include "suggestion_overlay.h"

constexpr UINT WM_QUIT_APP = WM_USER + 1;

// Event data structures
enum class EventType {
    KEYBOARD,
    MOUSE
};

struct KeyboardEventData {
    USHORT vKey;
    USHORT scanCode;
    USHORT flags;
    bool isKeyUp;
    
    KeyboardEventData() = default;
    KeyboardEventData(USHORT vk, USHORT sc, USHORT f, bool keyUp)
        : vKey(vk), scanCode(sc), flags(f), isKeyUp(keyUp) {}
};

struct MouseEventData {
    enum Type {
        LEFT_DOWN, LEFT_UP, RIGHT_DOWN, RIGHT_UP, 
        MIDDLE_DOWN, MIDDLE_UP, WHEEL, MOVE
    };
    
    Type eventType;
    LONG deltaX;
    LONG deltaY;
    short wheelData;  // Only used for wheel events
    
    MouseEventData() = default;
    MouseEventData(Type type, LONG dx, LONG dy, short wheel = 0)
        : eventType(type), deltaX(dx), deltaY(dy), wheelData(wheel) {}
};

struct InputEvent {
    std::uint64_t timestamp;
    POINT cursorPosition;
    EventType type;
    
    // Union to hold either keyboard or mouse data
    union {
        KeyboardEventData keyboardData;
        MouseEventData mouseData;
    };
    
    // Constructor for keyboard events
    InputEvent(std::uint64_t ts, POINT cursor, const KeyboardEventData& kbData)
        : timestamp(ts), cursorPosition(cursor), type(EventType::KEYBOARD), keyboardData(kbData) {}
        
    // Constructor for mouse events
    InputEvent(std::uint64_t ts, POINT cursor, const MouseEventData& mData)
        : timestamp(ts), cursorPosition(cursor), type(EventType::MOUSE), mouseData(mData) {}
        
    // Destructor (needed for union)
    ~InputEvent() {}
};



// Global variables
HWND g_hWnd = nullptr;
bool g_running = true;
std::vector<InputEvent> g_eventHistory;

// Get high-resolution timestamp
std::uint64_t GetTimestampMicros() {
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
}

// Helper functions for event storage
const std::vector<InputEvent>& GetEventHistory() {
    return g_eventHistory;
}

void ClearEventHistory() {
    g_eventHistory.clear();
}

size_t GetEventCount() {
    return g_eventHistory.size();
}

// Convert mouse event type to string
const char* MouseEventTypeToString(MouseEventData::Type type) {
    switch (type) {
        case MouseEventData::LEFT_DOWN: return "LEFT_DOWN";
        case MouseEventData::LEFT_UP: return "LEFT_UP";
        case MouseEventData::RIGHT_DOWN: return "RIGHT_DOWN";
        case MouseEventData::RIGHT_UP: return "RIGHT_UP";
        case MouseEventData::MIDDLE_DOWN: return "MIDDLE_DOWN";
        case MouseEventData::MIDDLE_UP: return "MIDDLE_UP";
        case MouseEventData::WHEEL: return "WHEEL";
        case MouseEventData::MOVE: return "MOVE";
        default: return "UNKNOWN";
    }
}

// Legacy functions removed - now using EventLogger

// Helper function to store event in memory and log using new EventLogger
void StoreEvent(std::uint64_t timestamp, POINT cursorPos, const KeyboardEventData& kbData) {
    InputEvent event(timestamp, cursorPos, kbData);
    g_eventHistory.emplace_back(event);
    
    // Use new EventLogger
    EventLogger::LogKeyboardEvent(timestamp, kbData.vKey, kbData.isKeyUp);
}

void StoreEvent(std::uint64_t timestamp, POINT cursorPos, const MouseEventData& mouseData) {
    InputEvent event(timestamp, cursorPos, mouseData);
    g_eventHistory.emplace_back(event);
    
    // Use new EventLogger for mouse clicks only
    std::string buttonName;
    bool isButtonUp = false;
    bool shouldLog = false;
    
    switch (mouseData.eventType) {
        case MouseEventData::LEFT_DOWN:
            buttonName = "left";
            isButtonUp = false;
            shouldLog = true;
            break;
        case MouseEventData::LEFT_UP:
            buttonName = "left";
            isButtonUp = true;
            shouldLog = true;
            break;
        case MouseEventData::RIGHT_DOWN:
            buttonName = "right";
            isButtonUp = false;
            shouldLog = true;
            break;
        case MouseEventData::RIGHT_UP:
            buttonName = "right";
            isButtonUp = true;
            shouldLog = true;
            break;
        case MouseEventData::MIDDLE_DOWN:
            buttonName = "middle";
            isButtonUp = false;
            shouldLog = true;
            break;
        case MouseEventData::MIDDLE_UP:
            buttonName = "middle";
            isButtonUp = true;
            shouldLog = true;
            break;
        // Skip WHEEL and MOVE events for simplified logging
        default:
            shouldLog = false;
            break;
    }
    
    if (shouldLog) {
        EventLogger::LogMouseButtonEvent(timestamp, buttonName, isButtonUp, cursorPos);
    }
}



// Process raw input data
void ProcessRawInput(HRAWINPUT hRawInput) {
    UINT dwSize = 0;
    
    // Get required buffer size
    GetRawInputData(hRawInput, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
    if (dwSize == 0) return;
    
    // Allocate buffer and get data
    auto lpb = std::make_unique<BYTE[]>(dwSize);
    if (GetRawInputData(hRawInput, RID_INPUT, lpb.get(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
        return;
    }
    
    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb.get());
    std::uint64_t timestamp = GetTimestampMicros();
    
    // Get current cursor position
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    
    if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        // Keyboard input
        RAWKEYBOARD& kb = raw->data.keyboard;
        bool isKeyUp = (kb.Flags & RI_KEY_BREAK) != 0;
        
        // Store keyboard event in memory
        KeyboardEventData kbData(kb.VKey, kb.MakeCode, kb.Flags, isKeyUp);
        StoreEvent(timestamp, cursorPos, kbData);
        
        // Check for ESC key to exit
        if (kb.VKey == VK_ESCAPE && !isKeyUp) {
            std::cout << "[" << std::setw(10) << timestamp << "us] ESC pressed - shutting down\n";
            g_running = false;
            PostMessage(g_hWnd, WM_QUIT_APP, 0, 0);
            return;
        }
        
        // Hide suggestion overlay on any key press (except when Left Ctrl is generating suggestion)
        if (!isKeyUp) { // Only on key down events
            bool isLeftCtrl = (kb.VKey == VK_CONTROL && (kb.Flags & RI_KEY_E0) == 0);
            bool isRightCtrl = (kb.VKey == VK_CONTROL && (kb.Flags & RI_KEY_E0) != 0);
            
            // Hide overlay for any key except Left Ctrl (which shows suggestions)
            // Right Ctrl will be handled by the special key handler
            if (!isLeftCtrl) {
                SuggestionOverlay::HideSuggestion();
            }
        }
        
        // Process special key events
        bool hookTriggered = SpecialKeyHandler::ProcessSpecialKeyEvent(kb.VKey, kb.Flags, isKeyUp, timestamp, cursorPos);
        
        // If this is not a special key and it's a key down event, notify the special key handler
        // (so it can track key combinations like Ctrl+A)
        if (!hookTriggered && !isKeyUp) {
            SpecialKeyHandler::NotifyRegularKeyPressed(kb.VKey);
        }
        
        // Normal console output (skip if hook was triggered to avoid spam)
        if (!hookTriggered) {
            const char* action = isKeyUp ? "UP" : "DOWN";
            std::cout << "[" << std::setw(10) << timestamp << "us] KB: VK=0x" 
                      << std::hex << std::setw(2) << std::setfill('0') << kb.VKey
                      << " SC=0x" << std::setw(2) << kb.MakeCode
                      << " " << action;
            if (SpecialKeyHandler::IsSpecialKey(kb.VKey)) {
                std::cout << " (" << SpecialKeyHandler::GetKeyName(kb.VKey) << ")";
            }
            std::cout << " Cursor=(" << std::dec << cursorPos.x << "," << cursorPos.y << ")\n";
        }
    }
    else if (raw->header.dwType == RIM_TYPEMOUSE) {
        // Mouse input
        RAWMOUSE& mouse = raw->data.mouse;
        
        // Store mouse event in memory and handle console output
        bool eventStored = false;
        
        // Button events
        if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
            StoreEvent(timestamp, cursorPos, MouseEventData(MouseEventData::LEFT_DOWN, mouse.lLastX, mouse.lLastY));
            std::cout << "[" << std::setw(10) << timestamp << "us] MOUSE: L_DOWN";
            eventStored = true;
        }
        else if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
            StoreEvent(timestamp, cursorPos, MouseEventData(MouseEventData::LEFT_UP, mouse.lLastX, mouse.lLastY));
            std::cout << "[" << std::setw(10) << timestamp << "us] MOUSE: L_UP";
            eventStored = true;
        }
        else if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
            StoreEvent(timestamp, cursorPos, MouseEventData(MouseEventData::RIGHT_DOWN, mouse.lLastX, mouse.lLastY));
            std::cout << "[" << std::setw(10) << timestamp << "us] MOUSE: R_DOWN";
            eventStored = true;
        }
        else if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
            StoreEvent(timestamp, cursorPos, MouseEventData(MouseEventData::RIGHT_UP, mouse.lLastX, mouse.lLastY));
            std::cout << "[" << std::setw(10) << timestamp << "us] MOUSE: R_UP";
            eventStored = true;
        }
        else if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
            StoreEvent(timestamp, cursorPos, MouseEventData(MouseEventData::MIDDLE_DOWN, mouse.lLastX, mouse.lLastY));
            std::cout << "[" << std::setw(10) << timestamp << "us] MOUSE: M_DOWN";
            eventStored = true;
        }
        else if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) {
            StoreEvent(timestamp, cursorPos, MouseEventData(MouseEventData::MIDDLE_UP, mouse.lLastX, mouse.lLastY));
            std::cout << "[" << std::setw(10) << timestamp << "us] MOUSE: M_UP";
            eventStored = true;
        }
        /*
        else if (mouse.usButtonFlags & RI_MOUSE_WHEEL) {
            short wheelDelta = static_cast<short>(mouse.usButtonData);
            StoreEvent(timestamp, cursorPos, MouseEventData(MouseEventData::WHEEL, mouse.lLastX, mouse.lLastY, wheelDelta));
            std::cout << "[" << std::setw(10) << timestamp << "us] MOUSE: WHEEL=" << wheelDelta;
            eventStored = true;
        }
        else if (mouse.lLastX != 0 || mouse.lLastY != 0) {
            // Movement only (don't spam for every tiny movement)
            static int moveCount = 0;
            if (++moveCount % 10 == 0) {  // Log every 10th movement
                StoreEvent(timestamp, cursorPos, MouseEventData(MouseEventData::MOVE, mouse.lLastX, mouse.lLastY));
                std::cout << "[" << std::setw(10) << timestamp << "us] MOUSE: MOVE";
                eventStored = true;
            } else {
                // Still store the event even if we don't log it to console
                StoreEvent(timestamp, cursorPos, MouseEventData(MouseEventData::MOVE, mouse.lLastX, mouse.lLastY));
                return;  // Skip console logging this movement
            }
        }*/
        else {
            return;  // No relevant mouse event
        }
        
        if (eventStored && (mouse.usButtonFlags || mouse.lLastX != 0 || mouse.lLastY != 0)) {
            std::cout << " Delta=(" << mouse.lLastX << "," << mouse.lLastY << ")"
                      << " Cursor=(" << cursorPos.x << "," << cursorPos.y << ")\n";
        }
    }
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INPUT:
        ProcessRawInput(reinterpret_cast<HRAWINPUT>(lParam));
        return 0;
        
    case WM_QUIT_APP:
        PostQuitMessage(0);
        return 0;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

// Register for raw input
bool RegisterRawInputDevices(HWND hWnd) {
    RAWINPUTDEVICE rid[2];
    
    // Register for keyboard input (0x01, 0x06)
    rid[0].usUsagePage = 0x01;    // Generic Desktop
    rid[0].usUsage = 0x06;        // Keyboard
    rid[0].dwFlags = RIDEV_INPUTSINK;  // Receive input even when not foreground
    rid[0].hwndTarget = hWnd;
    
    // Register for mouse input (0x01, 0x02)
    rid[1].usUsagePage = 0x01;    // Generic Desktop
    rid[1].usUsage = 0x02;        // Mouse
    rid[1].dwFlags = RIDEV_INPUTSINK;  // Receive input even when not foreground
    rid[1].hwndTarget = hWnd;
    
    return RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE));
}

// Function to demonstrate accessing stored event data
void PrintStoredEventsSummary() {
    std::cout << "\n=== STORED EVENTS SUMMARY ===\n";
    std::cout << "Total events stored: " << g_eventHistory.size() << "\n";
    
    size_t keyboardEvents = 0, mouseEvents = 0;
    size_t specialKeyPresses = 0;
    
    for (const auto& event : g_eventHistory) {
        if (event.type == EventType::KEYBOARD) {
            keyboardEvents++;
            // Check if this was a special key
            if (SpecialKeyHandler::IsSpecialKey(event.keyboardData.vKey)) {
                specialKeyPresses++;
            }
        } else {
            mouseEvents++;
        }
    }
    
    std::cout << "Keyboard events: " << keyboardEvents << "\n";
    std::cout << "Mouse events: " << mouseEvents << "\n";
    std::cout << "Special key events (Ctrl/Shift/Alt): " << specialKeyPresses << "\n";
    
    // Show last few events as example
    if (!g_eventHistory.empty()) {
        std::cout << "\nLast 5 events:\n";
        size_t start = g_eventHistory.size() > 5 ? g_eventHistory.size() - 5 : 0;
        for (size_t i = start; i < g_eventHistory.size(); ++i) {
            const auto& event = g_eventHistory[i];
            std::cout << "  [" << event.timestamp << "us] ";
            
            if (event.type == EventType::KEYBOARD) {
                const auto& kb = event.keyboardData;
                std::cout << "KB: VK=0x" << std::hex << kb.vKey << std::dec 
                          << " " << (kb.isKeyUp ? "UP" : "DOWN");
            } else {
                const auto& mouse = event.mouseData;
                std::cout << "MOUSE: ";
                switch (mouse.eventType) {
                    case MouseEventData::LEFT_DOWN: std::cout << "L_DOWN"; break;
                    case MouseEventData::LEFT_UP: std::cout << "L_UP"; break;
                    case MouseEventData::RIGHT_DOWN: std::cout << "R_DOWN"; break;
                    case MouseEventData::RIGHT_UP: std::cout << "R_UP"; break;
                    case MouseEventData::MIDDLE_DOWN: std::cout << "M_DOWN"; break;
                    case MouseEventData::MIDDLE_UP: std::cout << "M_UP"; break;
                    case MouseEventData::WHEEL: std::cout << "WHEEL=" << mouse.wheelData; break;
                    case MouseEventData::MOVE: std::cout << "MOVE"; break;
                }
                std::cout << " Delta=(" << mouse.deltaX << "," << mouse.deltaY << ")";
            }
            std::cout << " Cursor=(" << event.cursorPosition.x << "," << event.cursorPosition.y << ")\n";
        }
    }
    std::cout << "============================\n\n";
}

// Initialize the new event logging system
void InitializeEventLog() {
    EventLogger::Initialize();
    EventLogger::ClearLogFile();
}

int main() {
    std::cout << "WinOpAutoMouseKeybdtest - Global Input Capture Test\n";
    std::cout << "Features:\n";
    std::cout << "- Captures all keyboard and mouse events globally\n";
    std::cout << "- Stores structured event data in memory\n";
    std::cout << "- Saves events to 'input_events.txt' in simplified JSON format\n";
    std::cout << "- Special key hooks for: Ctrl, Shift, Alt keys\n";
    
    // Initialize the event log file
    InitializeEventLog();
    
    // Initialize the special key handler
    SpecialKeyHandler::Initialize();
    
    // Initialize the input injector
    InputInjector::Initialize();
    
    // Initialize the suggestion overlay
    SuggestionOverlay::Initialize();
    
    std::cout << "\nSpecial Key Hooks Active:\n";
    const auto& specialKeys = SpecialKeyHandler::GetSpecialKeys();
    for (size_t i = 0; i < specialKeys.size(); ++i) {
        std::cout << "- " << SpecialKeyHandler::GetKeyName(specialKeys[i]) << " (VK=0x" 
                  << std::hex << specialKeys[i] << std::dec << ")\n";
    }
    std::cout << "\nPress ESC to exit and view event summary...\n\n";
    
    // Register window class
    const wchar_t* className = L"WinOpAutoMouseKeybdtest";
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = className;
    
    if (!RegisterClassExW(&wc)) {
        std::cerr << "Failed to register window class\n";
        return 1;
    }
    
    // Create hidden window for message processing
    g_hWnd = CreateWindowExW(
        0, className, L"WinOpAutoMouseKeybdtest",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    if (!g_hWnd) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    
    // Register for raw input
    if (!RegisterRawInputDevices(g_hWnd)) {
        std::cerr << "Failed to register raw input devices\n";
        return 1;
    }
    
    std::cout << "Raw input registration successful. Listening for global input events...\n\n";
    
    // Message loop
    MSG msg;
    while (g_running && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup overlay
    SuggestionOverlay::Cleanup();
    
    // Show summary of stored events before exiting
    PrintStoredEventsSummary();
    
    std::cout << "[OK] All events have been saved to 'input_events.txt'\n";
    std::cout << "\nShutdown complete.\n";
    std::cout << "Press any key to close the console window...";
    std::cin.get();
    return 0;
}