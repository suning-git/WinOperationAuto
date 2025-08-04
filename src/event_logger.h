#pragma once

#include <windows.h>  
#include <string>
#include <cstdint>

class EventLogger {
public:
    // Initialize the event logger
    static void Initialize();
    
    // Clear/reset the log file
    static void ClearLogFile();
    
    // Log a keyboard event
    static void LogKeyboardEvent(std::uint64_t timestamp, USHORT vKey, bool isKeyUp);
    
    // Log a mouse button event  
    static void LogMouseButtonEvent(std::uint64_t timestamp, const std::string& button, bool isButtonUp, POINT cursorPos);
    
    // Set the log file path (default: "input_events.txt")
    static void SetLogFilePath(const std::string& filePath);

private:
    static std::string s_logFilePath;
    static bool s_initialized;
    static bool s_shiftPressed;
    static bool s_capsLockOn;
    
    // Convert virtual key to readable key name
    static std::string VKeyToKeyName(USHORT vKey);
    
    // Convert virtual key to character (considering shift state)
    static std::string VKeyToChar(USHORT vKey);
    
    // Check if a key is a printable character
    static bool IsPrintableKey(USHORT vKey);
    
    // Update modifier key states
    static void UpdateModifierStates(USHORT vKey, bool isKeyUp);
    
    // Write JSON entry to log file
    static void WriteLogEntry(const std::string& jsonEntry);
};