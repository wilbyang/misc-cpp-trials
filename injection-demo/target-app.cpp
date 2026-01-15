#include <windows.h>
#include <string>

const char* CLASS_NAME = "ProtectedWindowClass";
const char* WINDOW_TITLE = "Protected Window";

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Set background
            RECT rect;
            GetClientRect(hwnd, &rect);
            HBRUSH brush = CreateSolidBrush(RGB(240, 240, 240));
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);
            
            // Draw title
            SetTextColor(hdc, RGB(255, 0, 0));
            SetBkMode(hdc, TRANSPARENT);
            
            HFONT hFont = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                    DEFAULT_PITCH | FF_SWISS, "Arial");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            
            const char* text1 = "PROTECTED WINDOW";
            TextOut(hdc, 50, 50, text1, strlen(text1));
            
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
            
            // Draw info text
            hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                              DEFAULT_PITCH | FF_SWISS, "Arial");
            hOldFont = (HFONT)SelectObject(hdc, hFont);
            
            SetTextColor(hdc, RGB(0, 0, 0));
            const char* text2 = "This window uses WDA_EXCLUDEFROMCAPTURE for native protection.";
            TextOut(hdc, 50, 100, text2, strlen(text2));
            
            const char* text3 = "Modern tools (Win+Shift+S) cannot capture this window.";
            TextOut(hdc, 50, 130, text3, strlen(text3));
            
            const char* text4 = "DLL injection demos GDI BitBlt/StretchBlt hooking (legacy tools).";
            TextOut(hdc, 50, 160, text4, strlen(text4));
            
            // Display window handle for reference
            char handleText[100];
            sprintf(handleText, "Window Handle: 0x%p", hwnd);
            TextOut(hdc, 50, 200, handleText, strlen(handleText));
            
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Create window
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // Enable screenshot protection using SetWindowDisplayAffinity (Windows 10+)
    // This blocks Desktop Duplication API used by modern screenshot tools
    if (SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE)) {
        OutputDebugString("Screenshot protection enabled via WDA_EXCLUDEFROMCAPTURE\n");
    } else {
        OutputDebugString("Failed to enable WDA_EXCLUDEFROMCAPTURE\n");
    }
    
    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
