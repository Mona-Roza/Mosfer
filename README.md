# Mosfer

Mosfer is a minimal Qt Quick (QML) desktop application built with CMake and Qt 6.

## Overview

- Framework: Qt 6 (Quick)
- Language: C++ + QML
- Build system: CMake
- App target: `appMosfer`

Current UI is a basic `ApplicationWindow` (640x480) with the title "Hello World".

## Requirements

- Qt 6.8+ with `Qt6::Quick`
- CMake 3.16+
- A C++ compiler supported by your Qt kit

### Current Windows Toolchain (this workspace)

- Compiler: GNU g++ (MinGW)
- Compiler version: 15.2.0
- Compiler path: `C:/DevTools/w64devkit/bin/c++.exe`

## Build And Run

### Option 1: VS Code (recommended)

1. Open the project in VS Code.
2. Select a Qt/CMake kit compatible with Qt 6.
3. Configure and build with CMake Tools.
4. Run the generated target `appMosfer`.

### Option 2: Command line

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

Run the app:

```powershell
./build/appMosfer.exe
```

If your generator creates a config subfolder, the executable may instead be under `build/Debug/`.

## Project Structure

- `CMakeLists.txt`: Build configuration and Qt setup
- `main.cpp`: Qt application entry point and QML engine bootstrap
- `Main.qml`: Main QML UI
- `build/`: Local build output (generated files)

## Notes

- `cmake_minimum_required` is set to 3.16.
- The project uses `find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)`.
- The QML module URI is `Mosfer`, loaded using:

```cpp
engine.loadFromModule("Mosfer", "Main");
```