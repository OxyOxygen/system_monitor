#include "disk_monitor.h"

DiskInfo DiskMonitor::getDiskInfo(const std::string &drive) {
  DiskInfo info = {};

  ULARGE_INTEGER freeBytesAvailable;
  ULARGE_INTEGER totalBytes;
  ULARGE_INTEGER totalFreeBytes;

  if (GetDiskFreeSpaceExA(drive.c_str(), &freeBytesAvailable, &totalBytes,
                          &totalFreeBytes)) {
    info.totalSpace = totalBytes.QuadPart;
    info.freeSpace = totalFreeBytes.QuadPart;
    info.usedSpace = info.totalSpace - info.freeSpace;
    info.usagePercent = static_cast<double>(info.usedSpace) /
                        static_cast<double>(info.totalSpace) * 100.0;
  }

  return info;
}
