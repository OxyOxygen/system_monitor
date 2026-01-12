#include "cpu_monitor.h"
#include <iostream>

CpuMonitor::CpuMonitor()
    : cpuQuery(nullptr), cpuCounter(nullptr), initialized(false) {
  PDH_STATUS status;

  // Create PDH query
  status = PdhOpenQuery(nullptr, 0, &cpuQuery);
  if (status != ERROR_SUCCESS) {
    std::cerr << "Failed to open PDH query for CPU" << std::endl;
    return;
  }

  // Add CPU counter - Total Processor Time
  status = PdhAddEnglishCounter(
      cpuQuery, "\\Processor(_Total)\\% Processor Time", 0, &cpuCounter);
  if (status != ERROR_SUCCESS) {
    std::cerr << "Failed to add CPU counter" << std::endl;
    PdhCloseQuery(cpuQuery);
    return;
  }

  // Collect initial data (required for subsequent calls)
  PdhCollectQueryData(cpuQuery);

  initialized = true;
}

CpuMonitor::~CpuMonitor() {
  if (cpuQuery) {
    PdhCloseQuery(cpuQuery);
  }
}

double CpuMonitor::getCpuUsage() {
  if (!initialized) {
    return 0.0;
  }

  PDH_FMT_COUNTERVALUE counterValue;
  PDH_STATUS status;

  // Collect query data
  status = PdhCollectQueryData(cpuQuery);
  if (status != ERROR_SUCCESS) {
    return 0.0;
  }

  // Get formatted counter value
  status = PdhGetFormattedCounterValue(cpuCounter, PDH_FMT_DOUBLE, nullptr,
                                       &counterValue);
  if (status != ERROR_SUCCESS) {
    return 0.0;
  }

  return counterValue.doubleValue;
}
