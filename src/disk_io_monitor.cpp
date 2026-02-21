#include "disk_io_monitor.h"
#include <iostream>

DiskIOMonitor::DiskIOMonitor()
    : ioQuery(nullptr), readBytesCounter(nullptr), writeBytesCounter(nullptr),
      readOpsCounter(nullptr), writeOpsCounter(nullptr), initialized(false) {
  PDH_STATUS status;

  status = PdhOpenQuery(nullptr, 0, &ioQuery);
  if (status != ERROR_SUCCESS) {
    std::cerr << "DiskIOMonitor: Failed to open PDH query" << std::endl;
    return;
  }

  // Disk Read Bytes/sec (total across all physical disks)
  status = PdhAddEnglishCounter(ioQuery,
                                "\\PhysicalDisk(_Total)\\Disk Read Bytes/sec",
                                0, &readBytesCounter);
  if (status != ERROR_SUCCESS) {
    std::cerr << "DiskIOMonitor: Failed to add read bytes counter" << std::endl;
    PdhCloseQuery(ioQuery);
    return;
  }

  // Disk Write Bytes/sec
  status = PdhAddEnglishCounter(ioQuery,
                                "\\PhysicalDisk(_Total)\\Disk Write Bytes/sec",
                                0, &writeBytesCounter);
  if (status != ERROR_SUCCESS) {
    std::cerr << "DiskIOMonitor: Failed to add write bytes counter"
              << std::endl;
    PdhCloseQuery(ioQuery);
    return;
  }

  // Disk Reads/sec
  status = PdhAddEnglishCounter(
      ioQuery, "\\PhysicalDisk(_Total)\\Disk Reads/sec", 0, &readOpsCounter);
  if (status != ERROR_SUCCESS) {
    std::cerr << "DiskIOMonitor: Failed to add read ops counter" << std::endl;
    PdhCloseQuery(ioQuery);
    return;
  }

  // Disk Writes/sec
  status = PdhAddEnglishCounter(
      ioQuery, "\\PhysicalDisk(_Total)\\Disk Writes/sec", 0, &writeOpsCounter);
  if (status != ERROR_SUCCESS) {
    std::cerr << "DiskIOMonitor: Failed to add write ops counter" << std::endl;
    PdhCloseQuery(ioQuery);
    return;
  }

  // Collect initial data (required for rate-based counters)
  PdhCollectQueryData(ioQuery);
  initialized = true;
}

DiskIOMonitor::~DiskIOMonitor() {
  if (ioQuery) {
    PdhCloseQuery(ioQuery);
  }
}

DiskIOInfo DiskIOMonitor::getDiskIOInfo() {
  DiskIOInfo info = {};
  info.available = false;

  if (!initialized) {
    return info;
  }

  PDH_STATUS status = PdhCollectQueryData(ioQuery);
  if (status != ERROR_SUCCESS) {
    return info;
  }

  PDH_FMT_COUNTERVALUE val;

  status = PdhGetFormattedCounterValue(readBytesCounter, PDH_FMT_DOUBLE,
                                       nullptr, &val);
  if (status == ERROR_SUCCESS) {
    info.readBytesPerSec = val.doubleValue;
  }

  status = PdhGetFormattedCounterValue(writeBytesCounter, PDH_FMT_DOUBLE,
                                       nullptr, &val);
  if (status == ERROR_SUCCESS) {
    info.writeBytesPerSec = val.doubleValue;
  }

  status = PdhGetFormattedCounterValue(readOpsCounter, PDH_FMT_DOUBLE, nullptr,
                                       &val);
  if (status == ERROR_SUCCESS) {
    info.readOpsPerSec = val.doubleValue;
  }

  status = PdhGetFormattedCounterValue(writeOpsCounter, PDH_FMT_DOUBLE, nullptr,
                                       &val);
  if (status == ERROR_SUCCESS) {
    info.writeOpsPerSec = val.doubleValue;
  }

  info.available = true;
  return info;
}
