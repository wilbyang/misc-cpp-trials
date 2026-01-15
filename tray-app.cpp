#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <string>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_ICON 1
#define ID_HOTKEY 1
#define ID_BTN_QUIT 101

// Global variables
NOTIFYICONDATA nid = {};
HWND hButtonQuit = NULL;

void InitNotifyIconData(HWND hwnd) {
    memset(&nid, 0, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    _tcscpy_s(nid.szTip, _countof(nid.szTip), _T("My Tray App"));
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        // Register Hotkey: Ctrl + Alt + A
        if (!RegisterHotKey(hwnd, ID_HOTKEY, MOD_CONTROL | MOD_ALT, 'A')) {
            MessageBox(hwnd, _T("Failed to register hotkey (Ctrl + Alt + A)!"), _T("Error"), MB_ICONERROR);
        }

        // Initialize Tray Icon Data
        InitNotifyIconData(hwnd);

        // Create a Quit button
        hButtonQuit = CreateWindow(
            _T("BUTTON"),  // Predefined class; Unicode assumed 
            _T("Quit Application"),      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            100,         // x position 
            100,         // y position 
            150,         // Button width
            30,          // Button height
            hwnd,        // Parent window
            (HMENU)ID_BTN_QUIT,       // No menu.
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL);      // Pointer not needed.
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_QUIT) {
            DestroyWindow(hwnd);
        }
        break;

    case WM_CLOSE:
        // Minimize to tray instead of closing
        Shell_NotifyIcon(NIM_ADD, &nid);
        ShowWindow(hwnd, SW_HIDE);
        return 0; // Don't let default processing destroy the window

    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP) {
            // Restore window
            Shell_NotifyIcon(NIM_DELETE, &nid);
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
        }
        break;

    case WM_HOTKEY:
        if (wParam == ID_HOTKEY) {
            // Toggle visibility or just show
            if (IsWindowVisible(hwnd)) {
                // If visible, maybe minimize? Or just bring to front. 
                // Requirement says "wake up", usually implies showing.
                // Let's bring to front if visible, or restore if hidden.
                // If user wants hotkey to toggle, we could do that too.
                // For now, let's ensure it shows.
                SetForegroundWindow(hwnd);
            } else {
                Shell_NotifyIcon(NIM_DELETE, &nid);
                ShowWindow(hwnd, SW_SHOW);
                SetForegroundWindow(hwnd);
            }
        }
        break;

    case WM_DESTROY:
        UnregisterHotKey(hwnd, ID_HOTKEY);
        Shell_NotifyIcon(NIM_DELETE, &nid); // Ensure icon is removed
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"TrayAppWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Tray Application",            // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
