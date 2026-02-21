#pragma once

#include "gpu_monitor.h"
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

// ─── AI Process Info ─────────────────────────────────────────────────────────
struct AiProcessInfo {
  DWORD pid;
  std::string name;
  uint64_t memoryUsed; // Working set in bytes
};

// ─── Model Capability ────────────────────────────────────────────────────────
struct ModelCapability {
  std::string name;      // e.g. "Llama 3 8B"
  std::string category;  // e.g. "LLM", "Vision", "Diffusion"
  double requiredVramGB; // VRAM needed (quantized / FP16)
  bool canRun;           // true if enough free VRAM
};

// ─── AI Info ─────────────────────────────────────────────────────────────────
struct AiInfo {
  // AI processes
  std::vector<AiProcessInfo> processes;
  int totalAiProcesses;

  // GPU metrics for AI context
  std::string gpuName;
  double gpuUsagePercent; // Current GPU utilization
  uint64_t vramUsed;      // Bytes
  uint64_t vramTotal;     // Bytes
  double vramUsagePercent;
  uint64_t vramFree;     // Bytes
  double temperatureC;   // GPU temp (-1 if unavailable)
  double powerDrawWatts; // GPU power draw
  int tdpWatts;          // GPU TDP
  bool gpuAvailable;

  // AI power estimate
  double aiPowerEstimateW; // Estimated AI workload power

  // Model capabilities
  std::vector<ModelCapability> modelCapabilities;

  // Workload status
  std::string workloadStatus;     // "Active", "Idle", "No AI Workload"
  std::string workloadStatusDesc; // Detailed description

  // Environment detection
  bool cudaDetected;
  std::string cudaPath;
  bool pythonDetected;
  std::string pythonVersion;

  // Compute capability summary
  std::string computeSummary; // e.g. "24 GB VRAM • CUDA Ready • 3 AI Processes"
};

// ─── AI Monitor ──────────────────────────────────────────────────────────────
class AiMonitor {
public:
  AiMonitor();
  ~AiMonitor() = default;

  // Call every update cycle with current GPU info
  void update(const GpuInfo &gpuInfo);

  AiInfo getAiInfo() const;

private:
  AiInfo info;

  // Internal helpers
  void detectAiProcesses();
  void updateGpuMetrics(const GpuInfo &gpuInfo);
  void evaluateModelCapabilities();
  void determineWorkloadStatus();
  void detectEnvironment();
  void buildComputeSummary();

  // AI process name patterns
  static bool isAiProcess(const std::string &name);
};
