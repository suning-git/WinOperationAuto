#include "suggestion_overlay.h"
#include <iostream>
#include <wingdi.h>

// Static member definitions
HWND SuggestionOverlay::s_overlayWindow = nullptr;
bool SuggestionOverlay::s_initialized = false;
std::string SuggestionOverlay::s_currentSuggestion = "";

void SuggestionOverlay::Initialize() {
    if (s_initialized) return;
    
    if (CreateOverlayWindow()) {
        s_initialized = true;
        std::cout << "[OVERLAY] Suggestion overlay initialized\n";
    } else {
        std::cout << "[ERROR] Failed to create suggestion overlay\n";
    }
}

void SuggestionOverlay::ShowSuggestion(const std::string& suggestion) {
    if (!s_initialized || !s_overlayWindow) return;
    
    s_currentSuggestion = suggestion;
    
    // Update position near cursor
    UpdatePosition();
    
    // Show the window
    ShowWindow(s_overlayWindow, SW_SHOWNOACTIVATE);
    
    // Force redraw
    InvalidateRect(s_overlayWindow, nullptr, TRUE);
    UpdateWindow(s_overlayWindow);
    
    std::cout << "[OVERLAY] Showing suggestion: \"" << suggestion << "\"\n";
}

void SuggestionOverlay::HideSuggestion() {
    if (!s_initialized || !s_overlayWindow) return;
    
    ShowWindow(s_overlayWindow, SW_HIDE);
    s_currentSuggestion = "";
    
    std::cout << "[OVERLAY] Suggestion hidden\n";
}

void SuggestionOverlay::Cleanup() {
    if (s_overlayWindow) {
        DestroyWindow(s_overlayWindow);
        s_overlayWindow = nullptr;
    }
    s_initialized = false;
}

bool SuggestionOverlay::CreateOverlayWindow() {
    const wchar_t* className = L"SuggestionOverlay";
    
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = OverlayWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = className;
    wc.hbrBackground = CreateSolidBrush(RGB(45, 45, 48)); // Dark background
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    
    if (!RegisterClassExW(&wc)) {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            std::cout << "[ERROR] Failed to register overlay window class: " << error << "\n";
            return false;
        }
    }
    
    // Create the overlay window
    s_overlayWindow = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, // Always on top, no taskbar, no focus
        className,
        L"LLM Suggestion",
        WS_POPUP | WS_BORDER, // Popup window with border
        100, 100, 600, 50,    // Position and size (better height for single line)
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!s_overlayWindow) {
        std::cout << "[ERROR] Failed to create overlay window\n";
        return false;
    }
    
    return true;
}

void SuggestionOverlay::UpdatePosition() {
    if (!s_overlayWindow) return;
    
    // Get cursor position
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    
    // Position overlay slightly below and to the right of cursor
    int x = cursorPos.x + 20;
    int y = cursorPos.y + 30;
    
    // Get screen dimensions to avoid going off-screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Adjust if too close to screen edges (updated for 600x80 size)
    if (x + 600 > screenWidth) x = screenWidth - 620;
    if (y + 80 > screenHeight) y = cursorPos.y - 90;
    if (x < 0) x = 10;
    if (y < 0) y = 10;
    
    // Update window position
    SetWindowPos(s_overlayWindow, HWND_TOPMOST, x, y, 600, 80, 
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void SuggestionOverlay::DrawSuggestion(HDC hdc, RECT& rect) {
    // Set up drawing
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(220, 220, 220)); // Light gray text
    
    // Create font (1.5x bigger for even better readability)
    HFONT hFont = CreateFontW(
        33,                        // Height (22 * 1.5 = 33)
        0,                         // Width (auto)
        0,                         // Escapement
        0,                         // Orientation
        FW_NORMAL,                 // Weight
        FALSE,                     // Italic
        FALSE,                     // Underline
        FALSE,                     // StrikeOut
        DEFAULT_CHARSET,           // CharSet
        OUT_DEFAULT_PRECIS,        // OutputPrecision
        CLIP_DEFAULT_PRECIS,       // ClipPrecision
        CLEARTYPE_QUALITY,         // Quality (better for readability)
        DEFAULT_PITCH | FF_SWISS,  // PitchAndFamily
        L"Segoe UI"                // Font name
    );
    
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    
    // Add symbol prefix and draw text - try light bulb, fallback to simple symbol
    std::wstring wDisplayText = L"\U0001F4A1 ";  // Unicode light bulb emoji (💡)
    // If emoji doesn't work, you can replace above line with one of these:
    // std::wstring wDisplayText = L"\u2022 ";     // Bullet point (•)
    // std::wstring wDisplayText = L"\u25B6 ";     // Right triangle (▶)
    // std::wstring wDisplayText = L"AI: ";        // Simple text fallback
    
    // Convert suggestion to wide string and append
    std::wstring wSuggestion(s_currentSuggestion.begin(), s_currentSuggestion.end());
    wDisplayText += wSuggestion;
    
    // Draw with padding (reduced for smaller height)
    RECT textRect = rect;
    textRect.left += 10;
    textRect.top += 5;
    textRect.right -= 10;
    textRect.bottom -= 5;
    
    DrawTextW(hdc, wDisplayText.c_str(), -1, &textRect, 
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    
    // Cleanup
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

LRESULT CALLBACK SuggestionOverlay::OverlayWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        
        RECT rect;
        GetClientRect(hWnd, &rect);
        
        // Fill background with a subtle blue tint to indicate AI suggestion
        HBRUSH bgBrush = CreateSolidBrush(RGB(40, 50, 70));  // Dark blue-gray
        FillRect(hdc, &rect, bgBrush);
        DeleteObject(bgBrush);
        
        // Draw border with a subtle blue accent
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(70, 130, 180));
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        
        MoveToEx(hdc, 0, 0, nullptr);
        LineTo(hdc, rect.right - 1, 0);
        LineTo(hdc, rect.right - 1, rect.bottom - 1);
        LineTo(hdc, 0, rect.bottom - 1);
        LineTo(hdc, 0, 0);
        
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);
        
        // Draw suggestion text
        if (!s_currentSuggestion.empty()) {
            DrawSuggestion(hdc, rect);
        }
        
        EndPaint(hWnd, &ps);
        return 0;
    }
    
    case WM_DESTROY:
        return 0;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}