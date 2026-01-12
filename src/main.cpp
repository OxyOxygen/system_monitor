#include "cpu_monitor.h"
#include "disk_monitor.h"
#include "gpu_monitor.h"
#include "gui.h"
#include "memory_monitor.h"
#include "power_monitor.h"
#include <chrono>
#include <thread>


int main() {
  // Initialize GUI
  GUI gui;
  if (!gui.init("System Monitor", 1400, 600)) {
    return -1;
  }

  // Initialize monitors
  CpuMonitor cpuMonitor;
  MemoryMonitor memoryMonitor;
  DiskMonitor diskMonitor;
  GpuMonitor gpuMonitor;
  PowerMonitor powerMonitor;

  // Update interval (in seconds)
  constexpr double updateInterval = 1.0;
  auto lastUpdate = std::chrono::steady_clock::now();

  // Cached values
  double cpuUsage = 0.0;
  MemoryInfo memInfo = {};
  DiskInfo diskInfo = {};
  GpuInfo gpuInfo = {};
  PowerInfo powerInfo = {};

  // Main loop
  while (!gui.shouldClose()) {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = now - lastUpdate;

    // Update monitor data at specified interval
    if (elapsed.count() >= updateInterval) {
      cpuUsage = cpuMonitor.getCpuUsage();
      memInfo = memoryMonitor.getMemoryInfo();
      diskInfo = diskMonitor.getDiskInfo();
      gpuInfo = gpuMonitor.getGpuInfo();
      powerInfo = powerMonitor.getPowerInfo();

      lastUpdate = now;
    }

    // Render GUI
    gui.beginFrame();
    gui.render(cpuUsage, memInfo, diskInfo, gpuInfo, powerInfo);
    gui.endFrame();

    // Small sleep to prevent excessive CPU usage by the monitor itself
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
  }

  gui.cleanup();
  return 0;
}
