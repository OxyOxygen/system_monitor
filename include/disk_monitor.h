#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

struct DiskInfo {
  uint64_t totalSpace; // Total disk space in bytes
  uint64_t freeSpace;  // Free disk space in bytes
  uint64_t usedSpace;  // Used disk space in bytes
  double usagePercent; // Usage percentage
};

struct DiskDriveInfo {
  std::string driveLetter; // e.g. "C:"
  std::string volumeLabel; // e.g. "Windows", "Data"
  std::string driveType;   // "Fixed", "Removable", "Network", "CD-ROM"
  uint64_t totalSpace;
  uint64_t freeSpace;
  uint64_t usedSpace;
  double usagePercent;
};

class DiskMonitor {
public:
  DiskMonitor() = default;
  ~DiskMonitor() = default;

  DiskInfo getDiskInfo(const std::string &drive = "C:\\");
  std::vector<DiskDriveInfo> getAllDrives();
};
