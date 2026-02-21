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

std::vector<DiskDriveInfo> DiskMonitor::getAllDrives() {
  std::vector<DiskDriveInfo> drives;

  char driveStrings[512];
  DWORD len = GetLogicalDriveStringsA(sizeof(driveStrings), driveStrings);
  if (len == 0 || len > sizeof(driveStrings)) {
    return drives;
  }

  char *current = driveStrings;
  while (*current) {
    std::string drivePath(current); // e.g. "C:\"

    UINT type = GetDriveTypeA(drivePath.c_str());

    // Skip unknown and no-root-dir types
    if (type == DRIVE_UNKNOWN || type == DRIVE_NO_ROOT_DIR) {
      current += drivePath.size() + 1;
      continue;
    }

    DiskDriveInfo di = {};
    di.driveLetter = drivePath.substr(0, 2); // "C:"

    // Drive type string
    switch (type) {
    case DRIVE_REMOVABLE:
      di.driveType = "Removable";
      break;
    case DRIVE_FIXED:
      di.driveType = "Fixed";
      break;
    case DRIVE_REMOTE:
      di.driveType = "Network";
      break;
    case DRIVE_CDROM:
      di.driveType = "CD-ROM";
      break;
    case DRIVE_RAMDISK:
      di.driveType = "RAM Disk";
      break;
    default:
      di.driveType = "Unknown";
      break;
    }

    // Volume label
    char volumeName[256] = {};
    GetVolumeInformationA(drivePath.c_str(), volumeName, sizeof(volumeName),
                          nullptr, nullptr, nullptr, nullptr, 0);
    di.volumeLabel = volumeName;
    if (di.volumeLabel.empty()) {
      di.volumeLabel = "Local Disk";
    }

    // Space info
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceExA(drivePath.c_str(), &freeBytesAvailable, &totalBytes,
                            &totalFreeBytes)) {
      di.totalSpace = totalBytes.QuadPart;
      di.freeSpace = totalFreeBytes.QuadPart;
      di.usedSpace = di.totalSpace - di.freeSpace;
      if (di.totalSpace > 0) {
        di.usagePercent = static_cast<double>(di.usedSpace) /
                          static_cast<double>(di.totalSpace) * 100.0;
      }
    }

    drives.push_back(di);
    current += drivePath.size() + 1;
  }

  return drives;
}
