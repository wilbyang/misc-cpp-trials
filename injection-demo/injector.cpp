#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

// Find process ID by window title
DWORD FindProcessByWindow(const char* windowTitle) {
    HWND hwnd = FindWindow(NULL, windowTitle);
    if (!hwnd) {
        std::cerr << "Window not found: " << windowTitle << std::endl;
        return 0;
    }
    
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    
    if (processId == 0) {
        std::cerr << "Failed to get process ID for window" << std::endl;
        return 0;
    }
    
    std::cout << "Found window \"" << windowTitle << "\" with PID: " << processId << std::endl;
    return processId;
}

// Inject DLL using CreateRemoteThread technique
bool InjectDLL(DWORD processId, const char* dllPath) {
    std::cout << "\n=== Starting DLL Injection ===" << std::endl;
    std::cout << "Target PID: " << processId << std::endl;
    std::cout << "DLL Path: " << dllPath << std::endl;
    
    // Open target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        std::cerr << "Note: Run as Administrator for injection to work!" << std::endl;
        return false;
    }
    std::cout << "Process opened successfully" << std::endl;
    
    // Allocate memory in target process
    size_t dllPathLen = strlen(dllPath) + 1;
    LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL, dllPathLen, 
                                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteBuf) {
        std::cerr << "Failed to allocate memory in target process. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }
    std::cout << "Memory allocated at: 0x" << pRemoteBuf << std::endl;
    
    // Write DLL path to target process memory
    SIZE_T bytesWritten;
    if (!WriteProcessMemory(hProcess, pRemoteBuf, dllPath, dllPathLen, &bytesWritten)) {
        std::cerr << "Failed to write DLL path to target process. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    std::cout << "DLL path written to target process (" << bytesWritten << " bytes)" << std::endl;
    
    // Get address of LoadLibraryA
    HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
    if (!hKernel32) {
        std::cerr << "Failed to get kernel32.dll handle" << std::endl;
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    LPVOID pLoadLibrary = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryA");
    if (!pLoadLibrary) {
        std::cerr << "Failed to get LoadLibraryA address" << std::endl;
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    std::cout << "LoadLibraryA address: 0x" << pLoadLibrary << std::endl;
    
    // Create remote thread to load DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
                                       (LPTHREAD_START_ROUTINE)pLoadLibrary,
                                       pRemoteBuf, 0, NULL);
    if (!hThread) {
        std::cerr << "Failed to create remote thread. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    std::cout << "Remote thread created successfully" << std::endl;
    
    // Wait for thread to complete (with timeout)
    std::cout << "Waiting for DLL to load..." << std::endl;
    DWORD waitResult = WaitForSingleObject(hThread, 5000);  // 5 second timeout
    
    if (waitResult == WAIT_TIMEOUT) {
        std::cerr << "Timeout waiting for DLL to load!" << std::endl;
        std::cerr << "The remote thread may be stuck or failed to execute." << std::endl;
        std::cerr << "Possible causes:" << std::endl;
        std::cerr << "- DLL has dependencies that aren't available" << std::endl;
        std::cerr << "- DEP (Data Execution Prevention) blocking execution" << std::endl;
        std::cerr << "- The DLL's DllMain is hanging" << std::endl;
        TerminateThread(hThread, 1);
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    if (waitResult != WAIT_OBJECT_0) {
        std::cerr << "Wait failed with error: " << GetLastError() << std::endl;
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    // Get thread exit code (HMODULE of loaded DLL)
    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);
    
    if (exitCode == 0) {
        std::cerr << "DLL failed to load (LoadLibrary returned NULL)" << std::endl;
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    std::cout << "DLL loaded successfully! Module handle: 0x" << std::hex << exitCode << std::dec << std::endl;
    
    // Cleanup
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    
    std::cout << "\n=== DLL Injection Successful ===" << std::endl;
    return true;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   DLL Injection Tool" << std::endl;
    std::cout << "   Anti-Screenshot Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Find target process by window title
    const char* windowTitle = "Protected Window";
    std::cout << "Searching for window: \"" << windowTitle << "\"" << std::endl;
    
    DWORD pid = FindProcessByWindow(windowTitle);
    if (pid == 0) {
        std::cerr << "\nError: Target window not found!" << std::endl;
        std::cerr << "Make sure target-app.exe is running first." << std::endl;
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
    
    // Get full path to hook DLL
    char dllPath[MAX_PATH];
    GetFullPathName("hook-dll.dll", MAX_PATH, dllPath, NULL);
    
    // Check if DLL exists
    DWORD fileAttr = GetFileAttributes(dllPath);
    if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        std::cerr << "\nError: hook-dll.dll not found!" << std::endl;
        std::cerr << "Expected path: " << dllPath << std::endl;
        std::cerr << "Make sure the DLL is in the same directory as the injector." << std::endl;
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
    
    // Inject the DLL
    if (InjectDLL(pid, dllPath)) {
        std::cout << "\nInjection complete!" << std::endl;
        std::cout << "\nInstructions:" << std::endl;
        std::cout << "1. Try taking a screenshot of the Protected Window" << std::endl;
        std::cout << "2. Use Snipping Tool, Print Screen, or any screenshot tool" << std::endl;
        std::cout << "3. The protected window should appear black in screenshots" << std::endl;
        std::cout << "\nNote: Debug output can be viewed with DebugView tool" << std::endl;
    } else {
        std::cerr << "\nInjection failed!" << std::endl;
        std::cerr << "Common issues:" << std::endl;
        std::cerr << "- Not running as Administrator" << std::endl;
        std::cerr << "- Target process has different architecture (x86 vs x64)" << std::endl;
        std::cerr << "- Anti-virus blocking injection" << std::endl;
    }
    
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}
