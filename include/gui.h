#pragma once

#include "ai_monitor.h"
#include "cpu_monitor.h"
#include "disk_monitor.h"
#include "energy_monitor.h"
#include "gpu_benchmark.h"
#include "gpu_monitor.h"
#include "memory_monitor.h"
#include "network_monitor.h"
#include "nvml_monitor.h"
#include "power_monitor.h"
#include "process_monitor.h"
#include "system_info.h"

#include "imgui.h"
#include <string>
#include <vector>

struct GLFWwindow;

enum class Tab {
  Overview = 0,
  Processes,
  Network,
  Energy,
  AI,
  GpuCompute,
  System
};

class GUI {
public:
  GUI();
  ~GUI();

  bool init(const char *title, int width, int height);
  bool shouldClose();
  void beginFrame();
  void render(double cpuUsage, const std::vector<double> &coreUsages,
              const MemoryInfo &memInfo, const DiskInfo &diskInfo,
              const GpuInfo &gpuInfo, const PowerInfo &powerInfo,
              const std::vector<ProcessInfo> &processes,
              const NetworkInfo &netInfo, const SystemInfo &sysInfo,
              const EnergyInfo &energyInfo, const AiInfo &aiInfo,
              const NvmlInfo &nvmlInfo, const BenchmarkResult &benchResult);
  void endFrame();
  void cleanup();

private:
  GLFWwindow *window;
  bool initialized;
  Tab currentTab;

  // History buffers for graphs (last 60 samples)
  static constexpr int HISTORY_SIZE = 60;
  float cpuHistory[HISTORY_SIZE];
  float memHistory[HISTORY_SIZE];
  float gpuHistory[HISTORY_SIZE];
  float downloadHistory[HISTORY_SIZE];
  float uploadHistory[HISTORY_SIZE];
  float powerHistory[HISTORY_SIZE]; // Watts history for energy tab
  int historyOffset;

  // Smooth animation values
  float animCpu;
  float animMem;
  float animDisk;
  float animGpu;
  float animBattery;

  // Render helpers
  void setupStyle();
  void renderSidebar(double cpuUsage, const MemoryInfo &memInfo,
                     const DiskInfo &diskInfo, const GpuInfo &gpuInfo,
                     const PowerInfo &powerInfo, const EnergyInfo &energyInfo);
  void renderOverviewTab(double cpuUsage, const std::vector<double> &coreUsages,
                         const MemoryInfo &memInfo, const DiskInfo &diskInfo,
                         const GpuInfo &gpuInfo, const PowerInfo &powerInfo,
                         const NetworkInfo &netInfo);
  void renderProcessesTab(const std::vector<ProcessInfo> &processes);
  void renderNetworkTab(const NetworkInfo &netInfo);
  void renderEnergyTab(const EnergyInfo &energyInfo, const GpuInfo &gpuInfo);
  void renderAiTab(const AiInfo &aiInfo);
  void renderGpuComputeTab(const NvmlInfo &nvmlInfo,
                           const BenchmarkResult &benchResult);
  void renderSystemTab(const SystemInfo &sysInfo);

  // Custom widgets
  void renderCircularGauge(const char *label, float fraction, ImVec4 color,
                           float radius);
  void renderGraph(const char *label, const float *data, int dataSize,
                   int offset, float maxVal, ImVec4 color, ImVec2 size);
  void renderMiniBar(const char *label, float fraction, ImVec4 color);
  void renderInfoRow(const char *label, const char *value, ImVec4 valueColor);

  // Utilities
  void updateHistory(float cpuVal, float memVal, float gpuVal, float dlVal,
                     float ulVal, float wattsVal);
  static std::string formatBytes(uint64_t bytes);
  static std::string formatSpeed(double bytesPerSec);
  static std::string formatUptime(uint64_t seconds);
};
