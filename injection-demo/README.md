# Windows DLL Injection Anti-Screenshot Demo

This sub-project demonstrates Windows DLL injection using CreateRemoteThread technique to implement anti-screenshot protection by hooking GDI functions.

## Components

1. **target-app.exe** - Target application with a window to be protected
2. **hook-dll.dll** - Hook DLL that intercepts BitBlt/StretchBlt functions
3. **injector.exe** - Injector tool that performs DLL injection

## How It Works

### Injection Process
1. Target application creates a window titled "Protected Window"
2. Injector finds the target process by window title
3. Uses CreateRemoteThread to inject hook-dll.dll into the target process
4. Hook DLL installs inline hooks on BitBlt and StretchBlt functions
5. When screenshot tools try to capture the window, hooks intercept the calls
6. Protected window appears black in screenshots

### Hooking Mechanism
- Uses simple 5-byte JMP inline hooking
- Hooks intercept BitBlt and StretchBlt (GDI screenshot functions)
- When source DC belongs to protected window, fills destination with black
- Otherwise calls original function normally

## Building

From the root directory:
```bash
cmake -B build -S .
cmake --build build --config Release
```

Or build specific targets:
```bash
cmake --build build --target target-app
cmake --build build --target hook-dll
cmake --build build --target injector
```

## Usage

### Step 1: Run Target Application
```bash
.\build\Release\target-app.exe
```

This creates a window showing "PROTECTED WINDOW" and instructions.

### Step 2: Run Injector (as Administrator)
```bash
# Right-click -> Run as Administrator
.\build\Release\injector.exe
```

The injector will:
- Find the target window
- Inject hook-dll.dll into the target process
- Display confirmation when hooks are installed

### Step 3: Test Screenshot Protection
Try taking screenshots using:
- Windows Snipping Tool
- Print Screen key
- Third-party screenshot tools

The protected window should appear black or blank in all screenshots.

## Important Notes

### Administrator Privileges
- Injector requires Administrator privileges to access and modify other processes
- Right-click injector.exe and select "Run as Administrator"

### Architecture Matching
- x64 build requires x64 DLL and injector
- x86 build requires x86 DLL and injector
- Cross-architecture injection will fail

### Anti-Virus Software
- Some anti-virus software may block DLL injection
- Add exception if needed for testing purposes
- This is educational code, not malware

### Debug Output
- Hook DLL outputs debug messages using OutputDebugString
- Use DebugView tool (Sysinternals) to view debug output
- Shows when hooks are installed and when screenshots are blocked

## Technical Details

### CreateRemoteThread Injection Steps
1. OpenProcess with PROCESS_ALL_ACCESS
2. VirtualAllocEx to allocate memory in target process
3. WriteProcessMemory to write DLL path
4. GetProcAddress to get LoadLibraryA address
5. CreateRemoteThread with LoadLibraryA as entry point
6. Wait for thread completion

### Inline Hooking
- Saves original 5 bytes of target function
- Writes JMP instruction (0xE9 + relative offset)
- Changes memory protection with VirtualProtect
- Hook function checks if source is protected window
- Temporarily unhooks to call original function (prevents recursion)

### Hooked Functions
- **BitBlt** - Used by most screenshot tools
- **StretchBlt** - Used for scaled captures

## Limitations

- Only hooks BitBlt/StretchBlt (some tools use other methods)
- Does not protect against direct memory reading
- Does not protect against hardware capture devices
- Educational demonstration only, not production-ready

## Educational Purpose

This code demonstrates:
- Windows process injection techniques
- Function hooking mechanisms
- GDI API interception
- Memory protection and process interaction

**Disclaimer**: This is for educational purposes only. Use responsibly and legally.
