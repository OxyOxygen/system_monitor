#pragma once

#include <cstdint>
#include <pdh.h>
#include <windows.h>

struct GpuInfo {
  double gpuUsage;         // GPU utilization percentage
  uint64_t vramUsed;       // Used VRAM in bytes
  uint64_t vramTotal;      // Total VRAM in bytes
  double vramUsagePercent; // VRAM usage percentage
  bool available;          // GPU monitoring available
};

class GpuMonitor {
public:
  GpuMonitor();
  ~GpuMonitor();

  GpuInfo getGpuInfo();

private:
  PDH_HQUERY gpuQuery;
  PDH_HCOUNTER gpuCounter;
  PDH_HCOUNTER vramCounter;
  bool initialized;
};
