#pragma once

#include <pdh.h>
#include <windows.h>


class CpuMonitor {
public:
  CpuMonitor();
  ~CpuMonitor();

  double getCpuUsage();

private:
  PDH_HQUERY cpuQuery;
  PDH_HCOUNTER cpuCounter;
  bool initialized;
};
