#include "cpu_monitor.h"
#include <iostream>
#include <sstream>

CpuMonitor::CpuMonitor()
    : cpuQuery(nullptr), cpuCounter(nullptr), coreQuery(nullptr), numCores(0),
      initialized(false), coresInitialized(false) {
  PDH_STATUS status;

  // Get number of logical processors
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  numCores = sysInfo.dwNumberOfProcessors;

  // Create PDH query for total CPU
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

  // Setup per-core counters
  status = PdhOpenQuery(nullptr, 0, &coreQuery);
  if (status != ERROR_SUCCESS) {
    return;
  }

  coreCounters.resize(numCores, nullptr);
  bool allCoresOk = true;
  for (int i = 0; i < numCores; i++) {
    std::ostringstream counterPath;
    counterPath << "\\Processor(" << i << ")\\% Processor Time";
    status = PdhAddEnglishCounter(coreQuery, counterPath.str().c_str(), 0,
                                  &coreCounters[i]);
    if (status != ERROR_SUCCESS) {
      allCoresOk = false;
      break;
    }
  }

  if (allCoresOk) {
    PdhCollectQueryData(coreQuery);
    coresInitialized = true;
  } else {
    PdhCloseQuery(coreQuery);
    coreQuery = nullptr;
    coreCounters.clear();
  }
}

CpuMonitor::~CpuMonitor() {
  if (cpuQuery) {
    PdhCloseQuery(cpuQuery);
  }
  if (coreQuery) {
    PdhCloseQuery(coreQuery);
  }
}

#include <cmath>

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

  double usage = counterValue.doubleValue;
  if (std::isnan(usage) || std::isinf(usage)) usage = 0.0;
  if (usage < 0.0) usage = 0.0;
  if (usage > 100.0) usage = 100.0;
  
  return usage;
}

std::vector<double> CpuMonitor::getPerCoreUsage() {
  std::vector<double> coreUsages(numCores, 0.0);

  if (!coresInitialized) {
    return coreUsages;
  }

  PDH_STATUS status = PdhCollectQueryData(coreQuery);
  if (status != ERROR_SUCCESS) {
    return coreUsages;
  }

  for (int i = 0; i < numCores; i++) {
    PDH_FMT_COUNTERVALUE counterValue;
    status = PdhGetFormattedCounterValue(coreCounters[i], PDH_FMT_DOUBLE,
                                         nullptr, &counterValue);
    if (status == ERROR_SUCCESS) {
      coreUsages[i] = counterValue.doubleValue;
      if (coreUsages[i] > 100.0)
        coreUsages[i] = 100.0;
      if (coreUsages[i] < 0.0)
        coreUsages[i] = 0.0;
    }
  }

  return coreUsages;
}
