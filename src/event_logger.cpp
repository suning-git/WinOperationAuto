#include "event_logger.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Static member definitions
std::string EventLogger::s_logFilePath = "input_events.txt";
bool EventLogger::s_initialized = false;
bool EventLogger::s_shiftPressed = false;
bool EventLogger::s_capsLockOn = false;

void EventLogger::Initialize() {
    s_initialized = true;
    s_shiftPressed = false;
    
    // Check initial caps lock state
    s_capsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
    
    std::cout << "[OK] Event logger initialized (file: " << s_logFilePath << ")\n";
}

void EventLogger::ClearLogFile() {
    std::ofstream file(s_logFilePath, std::ios::trunc);
    if (file.is_open()) {
        file.close();
        std::cout << "[OK] Event log file cleared\n";
    } else {
        std::cout << "[WARNING] Could not clear log file\n";
    }
}

void EventLogger::LogKeyboardEvent(std::uint64_t timestamp, USHORT vKey, bool isKeyUp) {
    if (!s_initialized) {
        std::cerr << "[ERROR] EventLogger not initialized\n";
        return;
    }
    
    // Update modifier states first
    UpdateModifierStates(vKey, isKeyUp);
    
    std::string keyName = VKeyToKeyName(vKey);
    std::string charValue = VKeyToChar(vKey);
    std::string action = isKeyUp ? "keyup" : "keydown";
    
    // Build JSON entry
    std::ostringstream json;
    json << "{";
    json << "\"timestamp\":" << timestamp << ",";
    json << "\"type\":\"keyboard\",";
    json << "\"action\":\"" << action << "\",";
    json << "\"key\":\"" << keyName << "\"";
    
    if (!charValue.empty()) {
        json << ",\"char\":\"" << charValue << "\"";
    } else {
        json << ",\"char\":null";
    }
    
    json << "}";
    
    WriteLogEntry(json.str());
}

void EventLogger::LogMouseButtonEvent(std::uint64_t timestamp, const std::string& button, bool isButtonUp, POINT cursorPos) {
    if (!s_initialized) {
        std::cerr << "[ERROR] EventLogger not initialized\n";
        return;
    }
    
    std::string action = button + (isButtonUp ? "up" : "down");
    
    // Build JSON entry
    std::ostringstream json;
    json << "{";
    json << "\"timestamp\":" << timestamp << ",";
    json << "\"type\":\"mouse\",";
    json << "\"action\":\"" << action << "\",";
    json << "\"x\":" << cursorPos.x << ",";
    json << "\"y\":" << cursorPos.y;
    json << "}";
    
    WriteLogEntry(json.str());
}

void EventLogger::SetLogFilePath(const std::string& filePath) {
    s_logFilePath = filePath;
    std::cout << "[CONFIG] Log file path set to: " << filePath << "\n";
}

std::string EventLogger::VKeyToKeyName(USHORT vKey) {
    switch (vKey) {
        // Letters
        case 0x41: return "A"; case 0x42: return "B"; case 0x43: return "C"; case 0x44: return "D";
        case 0x45: return "E"; case 0x46: return "F"; case 0x47: return "G"; case 0x48: return "H";
        case 0x49: return "I"; case 0x4A: return "J"; case 0x4B: return "K"; case 0x4C: return "L";
        case 0x4D: return "M"; case 0x4E: return "N"; case 0x4F: return "O"; case 0x50: return "P";
        case 0x51: return "Q"; case 0x52: return "R"; case 0x53: return "S"; case 0x54: return "T";
        case 0x55: return "U"; case 0x56: return "V"; case 0x57: return "W"; case 0x58: return "X";
        case 0x59: return "Y"; case 0x5A: return "Z";
        
        // Numbers
        case 0x30: return "0"; case 0x31: return "1"; case 0x32: return "2"; case 0x33: return "3";
        case 0x34: return "4"; case 0x35: return "5"; case 0x36: return "6"; case 0x37: return "7";
        case 0x38: return "8"; case 0x39: return "9";
        
        // Special keys
        case VK_SPACE: return "SPACE";
        case VK_RETURN: return "ENTER";
        case VK_BACK: return "BACKSPACE";
        case VK_TAB: return "TAB";
        case VK_ESCAPE: return "ESC";
        case VK_DELETE: return "DELETE";
        case VK_INSERT: return "INSERT";
        case VK_HOME: return "HOME";
        case VK_END: return "END";
        case VK_PRIOR: return "PAGE_UP";
        case VK_NEXT: return "PAGE_DOWN";
        
        // Arrow keys
        case VK_UP: return "UP_ARROW";
        case VK_DOWN: return "DOWN_ARROW";
        case VK_LEFT: return "LEFT_ARROW";
        case VK_RIGHT: return "RIGHT_ARROW";
        
        // Modifier keys
        case VK_SHIFT: return "SHIFT";
        case VK_CONTROL: return "CTRL";
        case VK_MENU: return "ALT";
        case VK_CAPITAL: return "CAPS_LOCK";
        case VK_LWIN: return "LEFT_WIN";
        case VK_RWIN: return "RIGHT_WIN";
        
        // Function keys
        case VK_F1: return "F1"; case VK_F2: return "F2"; case VK_F3: return "F3";
        case VK_F4: return "F4"; case VK_F5: return "F5"; case VK_F6: return "F6";
        case VK_F7: return "F7"; case VK_F8: return "F8"; case VK_F9: return "F9";
        case VK_F10: return "F10"; case VK_F11: return "F11"; case VK_F12: return "F12";
        
        // Punctuation
        case VK_OEM_1: return "SEMICOLON";      // ;:
        case VK_OEM_PLUS: return "EQUALS";      // =+
        case VK_OEM_COMMA: return "COMMA";      // ,<
        case VK_OEM_MINUS: return "MINUS";      // -_
        case VK_OEM_PERIOD: return "PERIOD";    // .>
        case VK_OEM_2: return "SLASH";          // /?
        case VK_OEM_3: return "BACKTICK";       // `~
        case VK_OEM_4: return "LEFT_BRACKET";   // [{
        case VK_OEM_5: return "BACKSLASH";      // \|
        case VK_OEM_6: return "RIGHT_BRACKET";  // ]}
        case VK_OEM_7: return "QUOTE";          // '"
        
        default:
            // For unknown keys, return hex value
            std::ostringstream oss;
            oss << "VK_0x" << std::hex << vKey;
            return oss.str();
    }
}

std::string EventLogger::VKeyToChar(USHORT vKey) {
    // Only return characters for printable keys on key down events
    if (!IsPrintableKey(vKey)) {
        return "";  // Non-printable key
    }
    
    // Letters
    if (vKey >= 0x41 && vKey <= 0x5A) {
        char letter = 'A' + (vKey - 0x41);
        
        // Apply shift/caps lock logic
        bool shouldBeUppercase = s_shiftPressed ^ s_capsLockOn;  // XOR
        if (!shouldBeUppercase) {
            letter = letter - 'A' + 'a';  // Convert to lowercase
        }
        
        return std::string(1, letter);
    }
    
    // Numbers and their shifted symbols
    if (vKey >= 0x30 && vKey <= 0x39) {
        if (s_shiftPressed) {
            const char* shiftedNumbers = ")!@#$%^&*(";
            return std::string(1, shiftedNumbers[vKey - 0x30]);
        } else {
            return std::string(1, '0' + (vKey - 0x30));
        }
    }
    
    // Common punctuation
    switch (vKey) {
        case VK_SPACE: return " ";
        case VK_OEM_1: return s_shiftPressed ? ":" : ";";      // ;:
        case VK_OEM_PLUS: return s_shiftPressed ? "+" : "=";   // =+
        case VK_OEM_COMMA: return s_shiftPressed ? "<" : ",";  // ,<
        case VK_OEM_MINUS: return s_shiftPressed ? "_" : "-";  // -_
        case VK_OEM_PERIOD: return s_shiftPressed ? ">" : "."; // .>
        case VK_OEM_2: return s_shiftPressed ? "?" : "/";      // /?
        case VK_OEM_3: return s_shiftPressed ? "~" : "`";      // `~
        case VK_OEM_4: return s_shiftPressed ? "{" : "[";      // [{
        case VK_OEM_5: return s_shiftPressed ? "|" : "\\";     // \|
        case VK_OEM_6: return s_shiftPressed ? "}" : "]";      // ]}
        case VK_OEM_7: return s_shiftPressed ? "\"" : "'";     // '"
        default: return "";
    }
}

bool EventLogger::IsPrintableKey(USHORT vKey) {
    // Letters and numbers
    if ((vKey >= 0x41 && vKey <= 0x5A) || (vKey >= 0x30 && vKey <= 0x39)) {
        return true;
    }
    
    // Space and common punctuation
    switch (vKey) {
        case VK_SPACE:
        case VK_OEM_1: case VK_OEM_PLUS: case VK_OEM_COMMA: case VK_OEM_MINUS:
        case VK_OEM_PERIOD: case VK_OEM_2: case VK_OEM_3: case VK_OEM_4:
        case VK_OEM_5: case VK_OEM_6: case VK_OEM_7:
            return true;
        default:
            return false;
    }
}

void EventLogger::UpdateModifierStates(USHORT vKey, bool isKeyUp) {
    switch (vKey) {
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            s_shiftPressed = !isKeyUp;
            break;
        case VK_CAPITAL:
            if (!isKeyUp) {  // Only toggle on key down
                s_capsLockOn = !s_capsLockOn;
            }
            break;
    }
}

void EventLogger::WriteLogEntry(const std::string& jsonEntry) {
    std::ofstream file(s_logFilePath, std::ios::app);
    if (file.is_open()) {
        file << jsonEntry << std::endl;
        file.close();
    } else {
        std::cerr << "[ERROR] Could not write to log file: " << s_logFilePath << "\n";
    }
}