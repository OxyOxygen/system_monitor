#pragma once

#include <cstdint>
#include <pdh.h>
#include <windows.h>

struct DiskIOInfo {
  double readBytesPerSec;  // Disk read speed in bytes/sec
  double writeBytesPerSec; // Disk write speed in bytes/sec
  double readOpsPerSec;    // Read operations per second
  double writeOpsPerSec;   // Write operations per second
  bool available;          // Whether PDH counters are available
};

class DiskIOMonitor {
public:
  DiskIOMonitor();
  ~DiskIOMonitor();

  DiskIOInfo getDiskIOInfo();

private:
  PDH_HQUERY ioQuery;
  PDH_HCOUNTER readBytesCounter;
  PDH_HCOUNTER writeBytesCounter;
  PDH_HCOUNTER readOpsCounter;
  PDH_HCOUNTER writeOpsCounter;
  bool initialized;
};
