#pragma once

#include <windows.h>
#include <string>

class SuggestionOverlay {
public:
    // Initialize the overlay system
    static void Initialize();
    
    // Show suggestion on screen
    static void ShowSuggestion(const std::string& suggestion);
    
    // Hide the suggestion
    static void HideSuggestion();
    
    // Cleanup
    static void Cleanup();

private:
    static HWND s_overlayWindow;
    static bool s_initialized;
    static std::string s_currentSuggestion;
    
    // Window procedure for overlay
    static LRESULT CALLBACK OverlayWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    // Create the overlay window
    static bool CreateOverlayWindow();
    
    // Update window position (near cursor)
    static void UpdatePosition();
    
    // Draw the suggestion text
    static void DrawSuggestion(HDC hdc, RECT& rect);
};