#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

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
  void workerLoop();
  double queryWmiTemperature();
  double queryFallbackTemperature();

  CpuTempInfo cachedInfo;
  mutable std::mutex infoMutex;
  std::thread workerThread;
  std::atomic<bool> stopWorker{false};
  bool comInitialized;

  // Persistent WMI connection
  struct IWbemLocator* pLocator = nullptr;
  struct IWbemServices* pServices = nullptr;
  struct IWbemServices* pFallbackServices = nullptr;
  std::chrono::steady_clock::time_point lastUpdate;
};
