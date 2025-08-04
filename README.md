# WinOpAutoMouseKeybdtest

A Windows 11 x64 C++ console application that captures global keyboard and mouse input using the Raw Input API. This is a smoke test to confirm global input capture across all applications.

## Features

- **Global Input Capture**: Uses Raw Input API with `RIDEV_INPUTSINK` to capture input even when not in foreground
- **Keyboard Events**: Captures virtual keys, scan codes, and key up/down states
- **Mouse Events**: Captures button clicks, wheel events, and movement (throttled)
- **High-Resolution Timestamps**: Microsecond precision event logging
- **Clean Shutdown**: Press ESC to exit gracefully
- **Lightweight Logging**: Optimized for ≤100µs per event processing

## Requirements

- Windows 11 x64
- Visual Studio 2019/2022 with C++ tools
- CMake 3.20 or later
- Administrator privileges (required for `uiAccess="true"`)

## Build Instructions

### Using Visual Studio Developer Command Prompt

```cmd
# Clone/navigate to project directory
cd WinOpAuto_testMouseKeybd

# Create build directory
mkdir build
cd build

# Generate Visual Studio project files (x64)
cmake .. -G "Visual Studio 17 2022" -A x64

# Build the project
cmake --build . --config Release

# Alternative: Open in Visual Studio
# start WinOpAutoMouseKeybdtest.sln
```

### Using CMake directly

```cmd
# Create and enter build directory
mkdir build && cd build

# Configure for x64 Release build
cmake .. -DCMAKE_GENERATOR_PLATFORM=x64 -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release
```

## Run Instructions

**Important**: This application requires administrator privileges due to `uiAccess="true"` in the manifest.

```cmd
# From build directory, run as administrator
cd bin
./WinOpAutoMouseKeybdtest.exe
```

### Expected Output

```
WinOpAutoMouseKeybdtest - Global Input Capture Test
Press ESC to exit...

Raw input registration successful. Listening for global input events...

[    125043us] KB: VK=0x41 SC=0x1e DOWN Cursor=(1920,540)
[    125167us] KB: VK=0x41 SC=0x1e UP Cursor=(1920,540)
[    126891us] MOUSE: L_DOWN Delta=(0,0) Cursor=(1920,540)
[    127015us] MOUSE: L_UP Delta=(0,0) Cursor=(1920,540)
[    128456us] MOUSE: MOVE Delta=(2,1) Cursor=(1922,541)
```

## Architecture

- **Raw Input API**: No DLL injection or `SetWindowsHookEx` hooks
- **Console Application**: Lightweight, no GUI overhead  
- **Hidden Window**: Creates invisible window for message processing
- **Event Loop**: Standard Windows message pump with `WM_INPUT` handling
- **Efficient Logging**: Batched output with timestamp alignment

## Troubleshooting

### "Failed to register raw input devices"
- Ensure running as Administrator
- Check that the manifest is properly embedded
- Verify Windows version compatibility

### No events captured
- Confirm Administrator privileges
- Check if other security software is blocking input capture
- Verify the application is not being run in compatibility mode

### Build errors
- Ensure using x64 build configuration
- Verify C++20 compiler support
- Check that `user32.lib` is available in Windows SDK

## Technical Details

- **C++ Standard**: C++20 (`/std:c++20`)
- **Architecture**: x64 only
- **Compiler Flags**: `/W4` (warning level 4)
- **Dependencies**: `user32.lib` only
- **Manifest**: Embedded with `uiAccess="true"` and `requireAdministrator`
- **Input Devices**: 
  - Keyboard: Usage Page 0x01, Usage 0x06
  - Mouse: Usage Page 0x01, Usage 0x02

## License

This is a test/demonstration project. Use responsibly and in compliance with your local laws and regulations regarding input monitoring.