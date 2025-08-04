#pragma once

#include <windows.h>
#include <string>
#include <vector>

class InputInjector {
public:
    // Initialize the input injector
    static void Initialize();
    
    // Send a single virtual key (with optional key up)
    static bool SendVirtualKey(WORD vkCode, bool isKeyUp = false);
    
    // Send a complete key press (down + up)
    static bool SendKeyPress(WORD vkCode);
    
    // Send a text string (converts to appropriate key presses)
    static bool SendTextString(const std::string& text);
    
    // Send a sequence of virtual key codes
    static bool SendKeySequence(const std::vector<WORD>& vkCodes);
    
    // Utility: Convert character to virtual key code
    static WORD CharToVirtualKey(char c);
    
    // Add delay between key presses (in milliseconds)
    static void SetKeyDelay(DWORD delayMs);

private:
    static DWORD s_keyDelayMs;
    static bool s_initialized;
    
    // Helper to send raw INPUT structure
    static bool SendInputHelper(const INPUT& input);
};