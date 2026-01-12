# System Monitor

A modern desktop application for real-time system resource monitoring.

## Features

- CPU usage percentage
- RAM usage and total memory information
- Disk space usage (C: drive)
- GPU usage and VRAM tracking
- Battery status and power management
- Modern graphical user interface

## Requirements

### Software
- Windows 10 or later
- CMake 3.15 or later
- Visual Studio 2019/2022 or MinGW compiler
- OpenGL support (usually comes with the system)

### Hardware
- Any Windows computer
- Compatible GPU drivers for GPU monitoring (optional)

## Installation and Build

### Method 1: Using Build Script (Recommended)

```powershell
# Navigate to project directory
cd "d:/C_C++ Projects/System Monitor"

# Run build script
./build.ps1
```

### Method 2: Manual CMake

```powershell
# Navigate to project directory
cd "d:/C_C++ Projects/System Monitor"

# Generate build files
cmake -B build -G "Visual Studio 17 2022"

# Build project
cmake --build build --config Release

# Run the program
./build/Release/SystemMonitor.exe
```

### Method 3: Using Visual Studio

1. Open Visual Studio 2019 or 2022
2. Select File > Open > CMake
3. Choose the CMakeLists.txt file
4. Build > Build All (F7)
5. Debug > Start (F5)

## Usage

When the program starts, it displays five information panels:

- **CPU**: Real-time processor usage percentage
- **Memory**: Used and total RAM information
- **Disk**: C drive usage status
- **GPU**: Graphics card usage (if compatible)
- **Power**: Battery status, AC/Battery mode, remaining time

Information is automatically updated every second. Close the window to exit the program.

## Technical Details

### Technologies Used
- C++17 standard
- ImGui (Dear ImGui) - GUI framework
- GLFW - Window management
- OpenGL 3.0 - Graphics rendering
- Windows PDH API - CPU and GPU monitoring
- Windows API - Memory, disk, and power management

### Project Structure
```
System Monitor/
├── src/                    # Source code files
│   ├── main.cpp           # Main program
│   ├── cpu_monitor.cpp    # CPU monitoring
│   ├── memory_monitor.cpp # Memory monitoring
│   ├── disk_monitor.cpp   # Disk monitoring
│   ├── gpu_monitor.cpp    # GPU monitoring
│   ├── power_monitor.cpp  # Power monitoring
│   └── gui.cpp            # Graphical interface
├── include/               # Header files
├── external/              # External libraries
│   ├── imgui/            # ImGui library
│   └── glfw/             # GLFW library
└── CMakeLists.txt        # CMake configuration
```

## Troubleshooting

### "Windows.h not found" error
Try compiling using the MSVC compiler in Visual Studio. You can ignore IntelliSense errors in VS Code.

### GPU monitoring not working
GPU performance counters may not be available on some systems. The program will display a "not available" message in the GPU widget.

### CMake not found
If Visual Studio is installed, you can open the CMakeLists.txt file directly in Visual Studio.

## License

MIT License

## Developer Notes

This project was developed for learning purposes. It is suitable for practicing Windows system programming, GUI development, and real-time data monitoring.
