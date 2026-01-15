#include <windows.h>
#include <stdio.h>

// Global variables
static HWND g_ProtectedWindow = NULL;
static BOOL (WINAPI* TrueBitBlt)(HDC, int, int, int, int, HDC, int, int, DWORD) = BitBlt;
static BOOL (WINAPI* TrueStretchBlt)(HDC, int, int, int, int, HDC, int, int, int, int, DWORD) = StretchBlt;

// Simple inline hook structure
struct InlineHook {
    BYTE original[5];
    void* target;
    void* replacement;
};

static InlineHook g_BitBltHook = {};
static InlineHook g_StretchBltHook = {};

// Install a simple 5-byte JMP hook
BOOL InstallHook(InlineHook* hook, void* target, void* replacement) {
    hook->target = target;
    hook->replacement = replacement;
    
    // Save original bytes
    memcpy(hook->original, target, 5);
    
    // Calculate relative offset for JMP
    DWORD relativeAddr = (DWORD)((BYTE*)replacement - (BYTE*)target - 5);
    
    // Change memory protection
    DWORD oldProtect;
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return FALSE;
    }
    
    // Write JMP instruction (0xE9 = JMP rel32)
    BYTE jmp[5];
    jmp[0] = 0xE9;
    memcpy(&jmp[1], &relativeAddr, 4);
    memcpy(target, jmp, 5);
    
    // Restore protection
    VirtualProtect(target, 5, oldProtect, &oldProtect);
    
    return TRUE;
}

// Remove hook
BOOL RemoveHook(InlineHook* hook) {
    if (!hook->target) return FALSE;
    
    DWORD oldProtect;
    if (!VirtualProtect(hook->target, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return FALSE;
    }
    
    // Restore original bytes
    memcpy(hook->target, hook->original, 5);
    
    VirtualProtect(hook->target, 5, oldProtect, &oldProtect);
    
    return TRUE;
}

// Hooked BitBlt function
BOOL WINAPI HookedBitBlt(HDC hdcDest, int x, int y, int cx, int cy,
                         HDC hdcSrc, int x1, int y1, DWORD rop) {
    // Check if source DC belongs to our protected window
    HWND hwndSrc = WindowFromDC(hdcSrc);
    
    if (hwndSrc == g_ProtectedWindow || (g_ProtectedWindow && IsChild(g_ProtectedWindow, hwndSrc))) {
        // Block the screenshot attempt - fill with black instead
        HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        RECT rect = {x, y, x + cx, y + cy};
        FillRect(hdcDest, &rect, brush);
        DeleteObject(brush);
        
        OutputDebugString("BitBlt blocked for protected window!\n");
        return TRUE;  // Report success but don't actually copy
    }
    
    // Unhook temporarily to call original
    RemoveHook(&g_BitBltHook);
    BOOL result = BitBlt(hdcDest, x, y, cx, cy, hdcSrc, x1, y1, rop);
    InstallHook(&g_BitBltHook, (void*)BitBlt, (void*)HookedBitBlt);
    
    return result;
}

// Hooked StretchBlt function
BOOL WINAPI HookedStretchBlt(HDC hdcDest, int xDest, int yDest, int wDest, int hDest,
                             HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc, DWORD rop) {
    // Check if source DC belongs to our protected window
    HWND hwndSrc = WindowFromDC(hdcSrc);
    
    if (hwndSrc == g_ProtectedWindow || (g_ProtectedWindow && IsChild(g_ProtectedWindow, hwndSrc))) {
        // Block the screenshot attempt - fill with black instead
        HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        RECT rect = {xDest, yDest, xDest + wDest, yDest + hDest};
        FillRect(hdcDest, &rect, brush);
        DeleteObject(brush);
        
        OutputDebugString("StretchBlt blocked for protected window!\n");
        return TRUE;
    }
    
    // Unhook temporarily to call original
    RemoveHook(&g_StretchBltHook);
    BOOL result = StretchBlt(hdcDest, xDest, yDest, wDest, hDest, 
                            hdcSrc, xSrc, ySrc, wSrc, hSrc, rop);
    InstallHook(&g_StretchBltHook, (void*)StretchBlt, (void*)HookedStretchBlt);
    
    return result;
}

// Find the protected window by title
HWND FindProtectedWindow() {
    return FindWindow(NULL, "Protected Window");
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            // Disable thread notifications for optimization
            DisableThreadLibraryCalls(hModule);
            
            // Find the protected window
            g_ProtectedWindow = FindProtectedWindow();
            
            if (g_ProtectedWindow) {
                char msg[256];
                sprintf(msg, "Hook DLL loaded! Protected window: 0x%p\n", g_ProtectedWindow);
                OutputDebugString(msg);
                
                // Install hooks on BitBlt and StretchBlt
                if (InstallHook(&g_BitBltHook, (void*)BitBlt, (void*)HookedBitBlt)) {
                    OutputDebugString("BitBlt hooked successfully!\n");
                }
                
                if (InstallHook(&g_StretchBltHook, (void*)StretchBlt, (void*)HookedStretchBlt)) {
                    OutputDebugString("StretchBlt hooked successfully!\n");
                }
                
                // Don't use MessageBox in DllMain - it blocks the loader lock!
                // The hooks are now active
            } else {
                OutputDebugString("Warning: Protected window not found!\n");
            }
            break;
            
        case DLL_PROCESS_DETACH:
            // Remove hooks
            RemoveHook(&g_BitBltHook);
            RemoveHook(&g_StretchBltHook);
            OutputDebugString("Hook DLL unloaded, hooks removed.\n");
            break;
    }
    
    return TRUE;
}
