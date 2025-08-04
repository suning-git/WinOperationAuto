# WinOpAuto

Windows automation tool that captures input and provides AI-powered suggestions.

## Requirements
- Windows 11 x64
- Visual Studio with C++ tools
- CMake 3.20+
- Python 3.x
- Administrator privileges

## Build
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build .
```

## Run
```bash
# Install Python dependencies
pip install -r src/requirements.txt

# Run (as Administrator)
cd test
./WinOpAutoMouseKeybdtest.exe
```

## Components
- **C++**: Input capture, overlay system, event logging
- **Python**: LLM integration, input processing, text cleaning

Press ESC to exit.