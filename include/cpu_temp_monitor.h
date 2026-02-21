#pragma once

#include <string>
#include <windows.h>

struct CpuTempInfo {
  double temperatureC; // CPU temperature in Celsius
  bool isThrottling;   // True if temperature exceeds 90°C
  bool available;      // Whether temperature reading is available
};

class CpuTempMonitor {
public:
  CpuTempMonitor();
  ~CpuTempMonitor();

  void update();
  CpuTempInfo getInfo() const;

private:
  CpuTempInfo cachedInfo;
  bool comInitialized;

  double queryWmiTemperature();
};
