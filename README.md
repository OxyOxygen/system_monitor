# 🚀 AI Workstation & Performance Monitor v7.0

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%2F11-blueviolet.svg)](https://www.microsoft.com/windows)

**AI Workstation & Performance Monitor** is a professional-grade telemetry and diagnostic suite engineered for high-performance Windows systems. Designed for local AI development (LLMs, Stable Diffusion), heavy engineering workloads, and enthusiast gaming, this tool provides deep hardware visibility and performance intelligence.

---

## 📑 Table of Contents
- [Key Features](#-key-features)
- [Performance Analytics Hub](#-performance-analytics-hub)
- [System Intelligence Core](#-system-intelligence-core)
- [AI & ML Workstation Dashboard](#-ai--ml-workstation-dashboard)
- [Data Interoperability](#-data-interoperability)
- [Technical Architecture](#-technical-architecture)
- [Installation & Build](#-installation--build)

---

## 🏗️ Key Features

### 📊 Performance Analytics Hub
A dedicated suite for real-time tracking of system performance during intensive workloads.
- **Session Telemetry**: Captures FPS distribution, CPU/GPU pressure, and thermal signatures at 1Hz.
- **Latency Monitoring**: Integrated network latency (Ping) tracking to diagnose connection stability.
- **Visual Timelines**: Clear line charts showing simultaneous hardware utilization trends.
- **Proactive Alerts**: Color-coded status cards that flag thermal stress or system bottlenecks.

### 🧠 System Intelligence Core
An advanced heuristic analysis engine that provides deep diagnostic insights.
- **Bottleneck Identification**: Real-time detection of CPU, GPU, or VRAM limitations.
- **Intelligent Recommendations**: Actionable insights for system tuning and optimization.
- **Thermal Heuristics**: Analyzes temperature patterns to detect potential hardware throttling.

### ⚡ Energy & Economics
- **Real-time Wattage**: Detailed component-level power draw breakdown.
- **Operational Cost**: Session-based electricity cost tracking with monthly projections.

---

## 🧠 AI & ML Workstation Dashboard
Optimized for the modern AI developer, supporting over 30+ identification profiles.
- **AI Stack Detection**: Automatically recognizes PyTorch, TensorFlow, Ollama, and Stable Diffusion environments.
- **VRAM Capacity Estimator**: Calculates compatibility for 22+ popular LLMs (Llama 3, SDXL, etc.) based on hardware memory.
- **Env Validation**: Verification of CUDA readiness and Python availability.

---

## 📊 Data Interoperability
v7.0 introduces professional exports for data-driven performance tuning:
- **JSON Export**: Deep session metrics formatted for automated analysis pipelines.
- **CSV Export**: High-resolution timeline data (Timestamp, FPS, Latency, CPU, GPU).
- **Professional Reports**: Structured session summaries for quick performance debriefing.

---

## 🛠️ Technical Architecture

### Hardware Abstraction Layer (HAL)
The modular **HAL Architecture** (`IGpuProvider`) decouples the core UI from specific vendor APIs, ensuring a stable and extensible framework.
- **Native Support**: NVIDIA NVML, Windows PDH, DXGI.
- **Optimized Engine**: Asynchronous polling ensures monitoring has zero impact on primary workloads.

---

## 🚀 Installation & Build

### Prerequisites
- **Windows 10/11**
- **CMake 3.15+**
- **Visual Studio 2019/2022**

### Quick Start
```powershell
cmake -B build
cmake --build build --config Release
./build/Release/SystemMonitor.exe
```

---

## 📄 License
This project is licensed under the MIT License.

---
*Optimized for High-Performance Windows Workstations.*
