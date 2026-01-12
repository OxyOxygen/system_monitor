#include "memory_monitor.h"

MemoryInfo MemoryMonitor::getMemoryInfo() {
  MemoryInfo info = {};

  MEMORYSTATUSEX memStatus;
  memStatus.dwLength = sizeof(memStatus);

  if (GlobalMemoryStatusEx(&memStatus)) {
    info.totalRAM = memStatus.ullTotalPhys;
    info.freeRAM = memStatus.ullAvailPhys;
    info.usedRAM = info.totalRAM - info.freeRAM;
    info.usagePercent = static_cast<double>(info.usedRAM) /
                        static_cast<double>(info.totalRAM) * 100.0;
  }

  return info;
}
