#pragma once

#include <string>

struct FrameAnalysis {
  double estimatedFPS;    // Estimated frames per second
  double gpuFrameTimeMs;  // Estimated GPU frame time in ms
  double cpuFrameTimeMs;  // Estimated CPU frame time in ms
  bool gpuBound;          // True if GPU is the bottleneck
  bool cpuBound;          // True if CPU is the bottleneck
  std::string bottleneck; // "GPU Bound", "CPU Bound", "Balanced", "Idle"
  std::string status;     // "Gaming", "Light Load", "Idle"
};

class PerformanceAnalyzer {
public:
  PerformanceAnalyzer();
  ~PerformanceAnalyzer() = default;

  void update(double cpuUsage, double gpuUsage, double gpuClockMHz,
              double gpuMaxClockMHz);
  FrameAnalysis getAnalysis() const;

private:
  FrameAnalysis cachedAnalysis;

  // Smoothing
  double smoothCpu;
  double smoothGpu;
  static constexpr int HISTORY_SIZE = 10;
  double gpuUsageHistory[HISTORY_SIZE];
  int historyIdx;
};
