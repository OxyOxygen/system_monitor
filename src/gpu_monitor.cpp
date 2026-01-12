#include "gpu_monitor.h"
#include <iostream>

GpuMonitor::GpuMonitor()
    : gpuQuery(nullptr), gpuCounter(nullptr), vramCounter(nullptr),
      initialized(false) {
  PDH_STATUS status;

  // Create PDH query
  status = PdhOpenQuery(nullptr, 0, &gpuQuery);
  if (status != ERROR_SUCCESS) {
    std::cerr << "Failed to open PDH query for GPU" << std::endl;
    return;
  }

  // Try to add GPU usage counter
  // Note: This counter path may vary depending on GPU vendor and driver
  // Common paths:
  // NVIDIA: \\GPU Engine(*)\\Utilization Percentage
  // AMD: Similar pattern
  status = PdhAddEnglishCounter(
      gpuQuery, "\\GPU Engine(*)\\Utilization Percentage", 0, &gpuCounter);
  if (status != ERROR_SUCCESS) {
    // GPU monitoring not available (might not have compatible GPU/driver)
    std::cerr << "GPU monitoring not available - counter not found"
              << std::endl;
    PdhCloseQuery(gpuQuery);
    gpuQuery = nullptr;
    return;
  }

  // Try to add VRAM counter (optional, may not be available)
  PdhAddEnglishCounter(gpuQuery, "\\GPU Adapter Memory(*)\\Dedicated Usage", 0,
                       &vramCounter);

  // Collect initial data
  PdhCollectQueryData(gpuQuery);

  initialized = true;
}

GpuMonitor::~GpuMonitor() {
  if (gpuQuery) {
    PdhCloseQuery(gpuQuery);
  }
}

GpuInfo GpuMonitor::getGpuInfo() {
  GpuInfo info = {};
  info.available = initialized;

  if (!initialized) {
    return info;
  }

  PDH_FMT_COUNTERVALUE counterValue;
  PDH_STATUS status;

  // Collect query data
  status = PdhCollectQueryData(gpuQuery);
  if (status != ERROR_SUCCESS) {
    info.available = false;
    return info;
  }

  // Get GPU usage
  status = PdhGetFormattedCounterValue(gpuCounter, PDH_FMT_DOUBLE, nullptr,
                                       &counterValue);
  if (status == ERROR_SUCCESS) {
    info.gpuUsage = counterValue.doubleValue;
    // GPU Engine counter returns per-engine usage, often needs averaging
    // For simplicity, we'll use the value as-is
    if (info.gpuUsage > 100.0) {
      info.gpuUsage = 100.0;
    }
  }

  // Get VRAM usage (if available)
  if (vramCounter) {
    status = PdhGetFormattedCounterValue(vramCounter, PDH_FMT_LARGE, nullptr,
                                         &counterValue);
    if (status == ERROR_SUCCESS) {
      info.vramUsed = counterValue.largeValue;
      // Total VRAM is harder to get via PDH, might need different approach
      // For now, we'll estimate or leave it
      info.vramTotal = 8ULL * 1024 * 1024 * 1024; // Placeholder: 8GB
      info.vramUsagePercent = static_cast<double>(info.vramUsed) /
                              static_cast<double>(info.vramTotal) * 100.0;
    }
  }

  return info;
}
