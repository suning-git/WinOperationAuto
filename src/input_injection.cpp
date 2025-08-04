#include "input_injection.h"
#include <iostream>
#include <thread>
#include <chrono>

// Static member definitions
DWORD InputInjector::s_keyDelayMs = 10; // Default 10ms delay between keys
bool InputInjector::s_initialized = false;

void InputInjector::Initialize() {
    s_initialized = true;
    std::cout << "[OK] Input injector initialized with " << s_keyDelayMs << "ms key delay\n";
}

bool InputInjector::SendVirtualKey(WORD vkCode, bool isKeyUp) {
    if (!s_initialized) {
        std::cerr << "[ERROR] InputInjector not initialized\n";
        return false;
    }
    
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = isKeyUp ? KEYEVENTF_KEYUP : 0;
    input.ki.time = 0;
    
    return SendInputHelper(input);
}

bool InputInjector::SendKeyPress(WORD vkCode) {
    // Send key down, then key up
    bool success = SendVirtualKey(vkCode, false); // Key down
    if (success && s_keyDelayMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(s_keyDelayMs));
    }
    success &= SendVirtualKey(vkCode, true);      // Key up
    
    return success;
}

bool InputInjector::SendTextString(const std::string& text) {
    if (!s_initialized) {
        std::cerr << "[ERROR] InputInjector not initialized\n";
        return false;
    }
    
    std::cout << "[INPUT] Sending text: \"" << text << "\"\n";
    
    bool success = true;
    for (char c : text) {
        WORD vkCode = CharToVirtualKey(c);
        if (vkCode != 0) {
            // Check if we need Shift for this character
            SHORT vkScan = VkKeyScanA(c);
            WORD modifiers = HIBYTE(vkScan);
            
            // Press Shift if needed
            if (modifiers & 1) {
                SendVirtualKey(VK_SHIFT, false);
            }
            
            // Send the character
            success &= SendKeyPress(vkCode);
            
            // Release Shift if pressed
            if (modifiers & 1) {
                SendVirtualKey(VK_SHIFT, true);
            }
            
            // Add delay between characters
            if (s_keyDelayMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(s_keyDelayMs));
            }
        } else {
            std::cout << "[WARNING] Cannot convert character '" << c << "' to virtual key\n";
        }
    }
    
    return success;
}

bool InputInjector::SendKeySequence(const std::vector<WORD>& vkCodes) {
    if (!s_initialized) {
        std::cerr << "[ERROR] InputInjector not initialized\n";
        return false;
    }
    
    std::cout << "[INPUT] Sending key sequence of " << vkCodes.size() << " keys\n";
    
    bool success = true;
    for (WORD vkCode : vkCodes) {
        success &= SendKeyPress(vkCode);
        
        // Add delay between keys
        if (s_keyDelayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(s_keyDelayMs));
        }
    }
    
    return success;
}

WORD InputInjector::CharToVirtualKey(char c) {
    // Handle uppercase letters
    if (c >= 'A' && c <= 'Z') {
        return static_cast<WORD>('A' + (c - 'A'));
    }
    
    // Handle lowercase letters
    if (c >= 'a' && c <= 'z') {
        return static_cast<WORD>('A' + (c - 'a'));
    }
    
    // Handle digits
    if (c >= '0' && c <= '9') {
        return static_cast<WORD>('0' + (c - '0'));
    }
    
    // Handle space
    if (c == ' ') {
        return VK_SPACE;
    }
    
    // For other characters, try VkKeyScan
    SHORT vkScan = VkKeyScanA(c);
    if (vkScan != -1) {
        return LOBYTE(vkScan);
    }
    
    return 0; // Cannot convert
}

void InputInjector::SetKeyDelay(DWORD delayMs) {
    s_keyDelayMs = delayMs;
    std::cout << "[CONFIG] Key delay set to " << delayMs << "ms\n";
}

bool InputInjector::SendInputHelper(const INPUT& input) {
    UINT result = SendInput(1, const_cast<INPUT*>(&input), sizeof(INPUT));
    
    if (result != 1) {
        DWORD error = GetLastError();
        std::cerr << "[ERROR] SendInput failed with error: " << error << "\n";
        return false;
    }
    
    return true;
}