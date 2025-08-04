#pragma once

#include <windows.h>
#include <cstdint>
#include <vector>
#include <string>

// Special key state tracking
struct SpecialKeyState {
    bool isPressed;
    bool hadInterveningKeys;  // Flag to track if other keys were pressed while this special key was held
    std::uint64_t pressTimestamp;
    POINT pressPosition;
    
    SpecialKeyState() : isPressed(false), hadInterveningKeys(false), pressTimestamp(0), pressPosition({0, 0}) {}
};

// Special key management
class SpecialKeyHandler {
public:
    // Initialize the special key handler
    static void Initialize();
    
    // Check if a key is a special key we monitor
    static bool IsSpecialKey(USHORT vKey);
    
    // Get the index of a special key (-1 if not found)
    static int GetSpecialKeyIndex(USHORT vKey);
    
    // Get the name of a special key
    static const char* GetKeyName(USHORT vKey);
    
    // Process a special key event (returns true if it was handled)
    static bool ProcessSpecialKeyEvent(USHORT vKey, USHORT flags, bool isKeyUp, std::uint64_t timestamp, POINT cursorPos);
    
    // Notify that a regular key was pressed (to track key combinations)
    static void NotifyRegularKeyPressed(USHORT vKey);
    
    // Add a new special key to monitor
    static void AddSpecialKey(USHORT vKey);
    
    // Get list of monitored special keys
    static const std::vector<USHORT>& GetSpecialKeys();

private:
    // Event handlers for specific keys
    static void OnLeftCtrlPressed(std::uint64_t pressTime, std::uint64_t releaseTime, POINT pressPos, POINT releasePos);
    static void OnRightCtrlPressed(std::uint64_t pressTime, std::uint64_t releaseTime, POINT pressPos, POINT releasePos);
    static void OnShiftPressed(std::uint64_t pressTime, std::uint64_t releaseTime, POINT pressPos, POINT releasePos);
    static void OnAltPressed(std::uint64_t pressTime, std::uint64_t releaseTime, POINT pressPos, POINT releasePos);
    
    // Generic handler for any special key
    static void OnSpecialKeyPressed(USHORT vKey, std::uint64_t pressTime, std::uint64_t releaseTime, POINT pressPos, POINT releasePos);
    
    static std::vector<USHORT> s_specialKeys;
    static std::vector<SpecialKeyState> s_specialKeyStates;
    
    // State for suggestion workflow
    static std::string s_pendingSuggestion;
    static bool s_hasPendingSuggestion;
};