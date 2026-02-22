#include "ai_monitor.h"
#include "cpu_monitor.h"
#include "cpu_temp_monitor.h"
#include "disk_io_monitor.h"
#include "disk_monitor.h"
#include "energy_monitor.h"
#include "gpu_benchmark.h"
#include "gpu_monitor.h"
#include "gui.h"
#include "memory_monitor.h"
#include "network_monitor.h"
#include "nvml_monitor.h"
#include "performance_analyzer.h"
#include "performance_score.h"
#include "power_monitor.h"
#include "process_monitor.h"
#include "system_info.h"
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

int main() {
  // Initialize GUI
  GUI gui;
  if (!gui.init("System Monitor", 1600, 900)) {
    return -1;
  }

  // Initialize monitors
  CpuMonitor cpuMonitor;
  MemoryMonitor memoryMonitor;
  DiskMonitor diskMonitor;
  DiskIOMonitor diskIOMonitor;
  GpuMonitor gpuMonitor;
  PowerMonitor powerMonitor;
  ProcessMonitor processMonitor;
  NetworkMonitor networkMonitor;
  SystemInfoMonitor systemInfoMonitor;
  EnergyMonitor energyMonitor;
  AiMonitor aiMonitor;
  NvmlMonitor nvmlMonitor;
  GpuBenchmark gpuBenchmark;
  CpuTempMonitor cpuTempMonitor;
  PerformanceAnalyzer perfAnalyzer;
  PerformanceScore perfScore;

  // Initialize NVML (graceful if not available)
  nvmlMonitor.init();

  // Update interval (in seconds)
  constexpr double updateInterval = 1.0;
  auto lastUpdate = std::chrono::steady_clock::now();

  // Cached values
  double cpuUsage = 0.0;
  std::vector<double> coreUsages;
  MemoryInfo memInfo = {};
  DiskInfo diskInfo = {};
  std::vector<DiskDriveInfo> allDrives;
  DiskIOInfo diskIOInfo = {};
  GpuInfo gpuInfo = {};
  PowerInfo powerInfo = {};
  std::vector<ProcessInfo> processes;
  NetworkInfo netInfo = {};
  EnergyInfo energyInfo = {};
  AiInfo aiInfo = {};
  NvmlInfo nvmlInfo = {};
  BenchmarkResult benchResult = {};
  CpuTempInfo cpuTempInfo = {};
  FrameAnalysis frameAnalysis = {};
  ScoreInfo scoreInfo = {};
  bool benchmarkRan = false;
  bool scoreCalculated = false;

  // Main loop
  while (!gui.shouldClose()) {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = now - lastUpdate;

    // GAME MODE: Reduce polling rate if in game mode to save resources
    double effectiveInterval = gui.isGameMode() ? 2.0 : updateInterval;

    // Update monitor data at specified interval
    if (elapsed.count() >= effectiveInterval) {
      cpuUsage = cpuMonitor.getCpuUsage();
      coreUsages = cpuMonitor.getPerCoreUsage();
      memInfo = memoryMonitor.getMemoryInfo();
      diskInfo = diskMonitor.getDiskInfo();
      allDrives = diskMonitor.getAllDrives();
      diskIOInfo = diskIOMonitor.getDiskIOInfo();
      gpuInfo = gpuMonitor.getGpuInfo();
      powerInfo = powerMonitor.getPowerInfo();
      processes = processMonitor.getTopProcesses(15);
      netInfo = networkMonitor.getNetworkInfo();
      systemInfoMonitor.updateDynamic();

      // Update energy monitor with current CPU/GPU usage and TDP
      energyMonitor.update(cpuUsage, gpuInfo.gpuUsage, 0, gpuInfo.tdpWatts);
      energyInfo = energyMonitor.getEnergyInfo();

      // Update AI monitor with current GPU info
      aiMonitor.update(gpuInfo);
      aiInfo = aiMonitor.getAiInfo();

      // Update CPU temperature
      cpuTempMonitor.update();
      cpuTempInfo = cpuTempMonitor.getInfo();

      // Update NVML monitor
      if (nvmlMonitor.isAvailable()) {
        nvmlMonitor.update();
        nvmlInfo = nvmlMonitor.getInfo();

        // Auto-run benchmark once after first NVML update
        if (!benchmarkRan) {
          benchResult = gpuBenchmark.runBenchmark(nvmlInfo);
          benchmarkRan = true;
        }

        // Update performance analyzer with NVML clocks
        perfAnalyzer.update(cpuUsage, gpuInfo.gpuUsage, nvmlInfo.graphicsClock,
                            nvmlInfo.maxGraphicsClock);
      } else {
        perfAnalyzer.update(cpuUsage, gpuInfo.gpuUsage, 0, 0);
      }
      frameAnalysis = perfAnalyzer.getAnalysis();

      // Calculate performance score once (or periodically)
      if (!scoreCalculated) {
        perfScore.update(
            cpuMonitor.getNumCores(), 0.0, // Clock speed not easily available
            memInfo.totalRAM, memInfo.totalRAM - memInfo.usedRAM,
            gpuInfo.vramTotal, benchmarkRan ? benchResult.fp32Tflops : 0.0,
            nvmlInfo.cudaCapMajor, diskInfo.totalSpace, diskInfo.freeSpace);
        scoreInfo = perfScore.getScore();
        scoreCalculated = true;
      }

      lastUpdate = now;
    }

    // Render GUI
    gui.beginFrame();
    gui.render(cpuUsage, coreUsages, memInfo, diskInfo, gpuInfo, powerInfo,
               processes, netInfo, systemInfoMonitor.getInfo(), energyInfo,
               aiInfo, nvmlInfo, benchResult, allDrives, diskIOInfo,
               cpuTempInfo, frameAnalysis, scoreInfo, processMonitor);
    gui.endFrame();

    // Small sleep to prevent excessive CPU usage by the monitor itself
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
  }

  gui.cleanup();
  return 0;
}
