#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>


struct DiskInfo {
  uint64_t totalSpace; // Total disk space in bytes
  uint64_t freeSpace;  // Free disk space in bytes
  uint64_t usedSpace;  // Used disk space in bytes
  double usagePercent; // Usage percentage
};

class DiskMonitor {
public:
  DiskMonitor() = default;
  ~DiskMonitor() = default;

  DiskInfo getDiskInfo(const std::string &drive = "C:\\");
};
