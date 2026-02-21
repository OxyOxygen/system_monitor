#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>

struct ProcessInfo {
  DWORD pid;
  std::string name;
  double cpuUsage;     // CPU usage percentage
  uint64_t memoryUsed; // Working set size in bytes
};

// Internal per-process CPU time tracking
struct ProcessCpuData {
  ULARGE_INTEGER lastKernel;
  ULARGE_INTEGER lastUser;
};

class ProcessMonitor {
public:
  ProcessMonitor();
  ~ProcessMonitor() = default;

  std::vector<ProcessInfo> getTopProcesses(int count = 15);

  // Sort mode
  enum class SortMode { ByMemory, ByCpu };
  void setSortMode(SortMode mode) { sortMode = mode; }
  SortMode getSortMode() const { return sortMode; }

private:
  ULARGE_INTEGER lastSystemTime;
  ULARGE_INTEGER lastIdleTime;
  int numProcessors;

  // Per-process CPU tracking
  std::unordered_map<DWORD, ProcessCpuData> prevProcessTimes;
  ULARGE_INTEGER prevSystemTime;

  SortMode sortMode;
};
