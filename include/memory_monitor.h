#pragma once

#include <cstdint>
#include <windows.h>


struct MemoryInfo {
  uint64_t totalRAM;   // Total physical RAM in bytes
  uint64_t usedRAM;    // Used RAM in bytes
  uint64_t freeRAM;    // Free RAM in bytes
  double usagePercent; // Usage percentage
};

class MemoryMonitor {
public:
  MemoryMonitor() = default;
  ~MemoryMonitor() = default;

  MemoryInfo getMemoryInfo();
};
