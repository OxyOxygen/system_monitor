#pragma once

#include "ai_monitor.h"
#include "cpu_monitor.h"
#include "cpu_temp_monitor.h"
#include "disk_io_monitor.h"
#include "disk_monitor.h"
#include "energy_monitor.h"
#include "gpu_benchmark.h"
#include "gpu_monitor.h"
#include "memory_monitor.h"
#include "network_monitor.h"
#include "nvml_monitor.h"
#include "performance_analyzer.h"
#include "performance_score.h"
#include "power_monitor.h"
#include "process_monitor.h"
#include "system_info.h"
#include "hpc_engine.h"
#include "gaming_session.h"

#include "imgui.h"
#include <cstdint>
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
  System,
  HpcAssistant,
  GamingSession
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
              const NvmlInfo &nvmlInfo, const BenchmarkResult &benchResult,
              const std::vector<DiskDriveInfo> &allDrives,
              const DiskIOInfo &diskIOInfo, const CpuTempInfo &cpuTempInfo,
              const FrameAnalysis &frameAnalysis, const ScoreInfo &scoreInfo,
              const HpcReport &hpcReport,
              ProcessMonitor &processMonitor,
              GamingSessionMonitor &sessionMonitor, HpcEngine &hpcEngine);
  void endFrame();
  void cleanup();

  bool isGameMode() const { return gameMode; }
  void setGameMode(bool enabled) { gameMode = enabled; }

  bool isShowOverlay() const { return showOverlay; }
  void setShowOverlay(bool enabled) { showOverlay = enabled; }

private:
  GLFWwindow *window;
  bool initialized;
  Tab currentTab;
  bool gameMode;
  bool showOverlay;

  // Session state
  HpcReport hpcReportSession;
  bool showSessionReport = false;

  // History buffers for graphs (last 60 samples)
  static constexpr int HISTORY_SIZE = 60;
  float cpuHistory[HISTORY_SIZE];
  float memHistory[HISTORY_SIZE];
  float gpuHistory[HISTORY_SIZE];
  float downloadHistory[HISTORY_SIZE];
  float uploadHistory[HISTORY_SIZE];
  float powerHistory[HISTORY_SIZE];
  float diskReadHistory[HISTORY_SIZE];
  float diskWriteHistory[HISTORY_SIZE];
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
                     const PowerInfo &powerInfo, const EnergyInfo &energyInfo,
                     const ScoreInfo &scoreInfo);
  void renderOverviewTab(double cpuUsage, const std::vector<double> &coreUsages,
                         const MemoryInfo &memInfo, const DiskInfo &diskInfo,
                         const GpuInfo &gpuInfo, const PowerInfo &powerInfo,
                         const NetworkInfo &netInfo,
                         const std::vector<DiskDriveInfo> &allDrives,
                         const DiskIOInfo &diskIOInfo,
                         const CpuTempInfo &cpuTempInfo);
  void renderProcessesTab(const std::vector<ProcessInfo> &processes,
                          ProcessMonitor &processMonitor);
  void renderNetworkTab(const NetworkInfo &netInfo);
  void renderEnergyTab(const EnergyInfo &energyInfo, const GpuInfo &gpuInfo);
  void renderAiTab(const AiInfo &aiInfo);
  void renderGpuComputeTab(const NvmlInfo &nvmlInfo,
                           const BenchmarkResult &benchResult,
                           const FrameAnalysis &frameAnalysis);
  void renderSystemTab(const SystemInfo &sysInfo, const ScoreInfo &scoreInfo);
  void renderHpcAssistantTab(const HpcReport &hpcReport);
  void renderGamingSessionTab(GamingSessionMonitor &sessionMonitor, HpcEngine &hpcEngine);
  void renderOverlay(double cpuUsage, const MemoryInfo &memInfo,
                     const GpuInfo &gpuInfo, const CpuTempInfo &cpuTempInfo);

  // Custom widgets
  void renderCircularGauge(const char *label, float fraction, ImVec4 color,
                           float radius);
  void renderGraph(const char *label, const float *data, int dataSize,
                   int offset, float maxVal, ImVec4 color, ImVec2 size);
  void renderMiniBar(const char *label, float fraction, ImVec4 color);
  void renderInfoRow(const char *label, const char *value, ImVec4 valueColor);

  // Utilities
  void saveReportToTxt(const HpcReport &report, const SessionMetrics &metrics);
  void updateHistory(float cpuVal, float memVal, float gpuVal, float dlVal,
                     float ulVal, float wattsVal, float diskReadVal,
                     float diskWriteVal);
  static std::string formatBytes(uint64_t bytes);
  static std::string formatSpeed(double bytesPerSec);
  static std::string formatUptime(uint64_t seconds);
};
