# System Monitor v5.0

A modern desktop application for real-time system resource monitoring with **GPU Compute Dashboard**, **AI Engineer Dashboard**, **energy cost tracking**, and a premium dark UI. Built for AI/ML developers and GPU programmers who need deep hardware visibility.

## ⚡ Key Feature: Energy Cost Tracking

Unlike any other system monitor, this app estimates your **real-time electricity cost**:

- **Live power draw** (watts) for CPU, GPU, and total system
- **Session cost** — see how much you've spent since launching
- **Daily & monthly projections** — know your electricity bill impact
- **Power history graph** — visualize consumption over time
- **40+ GPU/CPU TDP profiles** — auto-detects your hardware for accurate estimates

Perfect for **AI/ML developers**, **gamers**, and anyone running heavy GPU workloads.

## 🧠 Key Feature: AI Engineer Dashboard

A dedicated tab designed for AI engineers and ML practitioners:

- **AI Process Detection** — automatically finds running AI/ML processes (Python, Ollama, PyTorch, TensorFlow, Stable Diffusion, ComfyUI, Whisper, LM Studio, and 30+ more)
- **GPU for AI** — real-time VRAM usage bar, GPU load, temperature, and power/TDP breakdown
- **Model Capacity Estimator** — shows which LLMs and AI models your GPU can run based on VRAM (22 models: Llama, Mistral, Mixtral, SDXL, FLUX, Whisper, etc.)
- **Workload Status** — Active / Idle / No AI Workload indicator with process count and power attribution
- **Environment Detection** — CUDA and Python availability check

## Features

### Overview Tab
- **CPU**: Total usage history graph + per-core status bars + [NEW] **Real-time Temperature** & Throttling status
- **Memory**: RAM usage tracking with allocation bars and live history
- **Disk I/O**: [NEW] **Per-drive activity graphs** (Read/Write speeds) and IOPS tracking
- **Drives**: [NEW] Multi-drive listing (C:, D:, etc.) with volume labels and usage %
- **Network**: Live Download/Upload speeds with peak tracking
- **Power**: Battery status, AC/Battery mode, and remaining time estimation

### Processes Tab
- Top 15 processes with real-time data
- [NEW] **Sort by CPU% or Memory Usage** toggle
- [NEW] **CPU Usage %** column for every process
- PID, process name, memory size, and visual usage bars
- Scrollable table with resizable columns

### Network Tab
- Real-time download/upload speed graphs
- IP address, MAC address, connection type, link speed
- Packet counters and active TCP connections
- Peak speed tracking

### ⚡ Energy Tab
- **Real-time wattage** — total system power estimate with component breakdown
- **Cost tracking** — session cost, energy used (Wh), session duration
- **Projections** — daily and monthly electricity cost estimates
- **Power graph** — live power consumption history
- **TDP info** — detected CPU & GPU TDP values

### 🧠 AI Tab
- **AI Workload Status** — Active/Idle/No AI Workload with glowing status indicator
- **AI Processes** — table of detected AI/ML processes (PID, name, memory)
- **GPU for AI** — VRAM bar (used/total/free), GPU load, temperature, power vs TDP
- **Model Capacity Estimator** — 22 models across LLM, Vision, Diffusion, Audio categories with OK/NO compatibility
- **Environment** — CUDA readiness, Python detection

### 🎮 Modern Performance & Gaming
- [NEW] **Game Mode** — Reduce polling frequency (from 1s to 2s) to minimize CPU impact while playing heavy games.
- [NEW] **Game Overlay** — Lightweight, semi-transparent info box that stays on top of games. Shows CPU/GPU load and temperature.
- [NEW] **Optimized Background Engine** — Reduced overhead by caching process names and optimizing NVML hardware queries.

### 🔥 GPU Compute & Gaming Tab
- [NEW] **Gaming Performance Dashboard** — Estimated FPS and frame time tracking (CPU vs GPU ms)
- [NEW] **Bottleneck Analysis** — High-precision detection for CPU or GPU limited scenarios
- **NVML Real-Time Metrics** — temperature, power, SM utilization, clocks (Graphics/Memory/SM), fan speed, and PCIe bandwidth
- **Compute Capabilities** — CUDA version, core count, architecture (Maxwell to Blackwell), and hardware features
- **Active GPU Processes** — lists all processes utilizing the GPU with VRAM usage
- **GPU Benchmark** — theoretical TFLOPS and measured VRAM bandwidth benchmarks
- **Graceful Fallback** — if NVML is unavailable (non-NVIDIA GPU), shows a clear message and estimated data continue from DXGI/PDH

### System & Performance Tab
- [NEW] **System Performance Score** — 0-100 score based on hardware tier and current metrics
- [NEW] **Letter Grades** — Automatic grading (S, A, B, C, D, F) for your system setup
- **Hardware Specs**: Computer name, OS version, CPU model, core count, and RAM physical info
- **Uptime**: Real-time system uptime counter

### Sidebar
- Circular gauge widgets for CPU, Memory, Disk, GPU, and Battery
- Live power draw display (watts) with session cost
- [NEW] **Toggle Controls** — Quickly enable/disable Game Mode and Overlay directly from the sidebar.
- Smooth animated value transitions

## Requirements

### Software
- Windows 10 or later
- CMake 3.15 or later
- Visual Studio 2019/2022 with C++ workload
- OpenGL support

### Hardware
- Any Windows computer
- Compatible GPU drivers for GPU monitoring (optional)

## Installation and Build

### Quick Build

```powershell
cd "System Monitor"
cmake -B build
cmake --build build --config Release
./build/Release/SystemMonitor.exe
```

### Using Visual Studio

1. Open Visual Studio
2. File > Open > CMake
3. Choose CMakeLists.txt
4. Build > Build All (F7)
5. Debug > Start (F5)

## Technical Details

### Technologies Used
- **C++17** standard
- **ImGui** (Dear ImGui) — custom drawn widgets with dark theme
- **GLFW** — window management
- **OpenGL 3.0** — graphics rendering
- **DXGI** — GPU hardware detection (name, VRAM)
- **Windows PDH API** — CPU and GPU performance counters
- **NVML** — NVIDIA GPU metrics (runtime loaded via `LoadLibrary`, no SDK required)
- **Direct3D 11** — GPU benchmark VRAM bandwidth measurement
- **Windows API** — memory, disk, power, process, network, registry
- **IP Helper API** — network traffic statistics

### Project Structure
```
System Monitor/
├── src/
│   ├── main.cpp              # Application entry point
│   ├── cpu_monitor.cpp       # CPU monitoring (total + per-core)
│   ├── memory_monitor.cpp    # Memory monitoring
│   ├── disk_monitor.cpp      # Disk monitoring
│   ├── gpu_monitor.cpp       # GPU monitoring (DXGI + PDH)
│   ├── power_monitor.cpp     # Battery/power status
│   ├── process_monitor.cpp   # Process enumeration
│   ├── network_monitor.cpp   # Network traffic monitoring
│   ├── system_info.cpp       # System information
│   ├── energy_monitor.cpp    # Energy cost tracking
│   ├── ai_monitor.cpp        # AI process detection & model capacity
│   ├── nvml_monitor.cpp      # NVML GPU metrics (dynamic loading)
│   ├── gpu_benchmark.cpp     # GPU benchmark (TFLOPS + bandwidth)
│   └── gui.cpp               # ImGui interface (7 tabs)
├── include/                   # Header files
├── external/                  # External libraries
│   ├── imgui/                # ImGui library
│   └── glfw/                 # GLFW library
└── CMakeLists.txt            # CMake build configuration
```

### Energy Estimation Formula

```
CPU Power = TDP × (0.15 + 0.75 × usage%)
GPU Power = TDP × (0.10 + 0.85 × usage%)
System Base = 50W (motherboard, RAM, drives)
Total = CPU + GPU + System Base
Cost = (Total × Time) / 1000 × Rate (per kWh)
```

## Troubleshooting

### GPU monitoring not working
GPU performance counters may not be available on some systems. The GPU name and VRAM will still be detected via DXGI.

### Energy estimates seem wrong
Energy values are **estimates** based on TDP and utilization percentage. Actual power varies by workload type, voltage, and thermals. The TDP lookup table covers 40+ popular GPUs and CPUs.

## License

MIT License
