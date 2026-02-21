#include "performance_analyzer.h"
#include <algorithm>
#include <cmath>
#include <cstring>

PerformanceAnalyzer::PerformanceAnalyzer()
    : smoothCpu(0.0), smoothGpu(0.0), historyIdx(0) {
  cachedAnalysis = {};
  cachedAnalysis.bottleneck = "Idle";
  cachedAnalysis.status = "Idle";
  std::memset(gpuUsageHistory, 0, sizeof(gpuUsageHistory));
}

void PerformanceAnalyzer::update(double cpuUsage, double gpuUsage,
                                 double gpuClockMHz, double gpuMaxClockMHz) {
  // Smooth values
  double alpha = 0.3;
  smoothCpu = smoothCpu * (1.0 - alpha) + cpuUsage * alpha;
  smoothGpu = smoothGpu * (1.0 - alpha) + gpuUsage * alpha;

  // Track GPU usage history for variance
  gpuUsageHistory[historyIdx % HISTORY_SIZE] = gpuUsage;
  historyIdx++;

  FrameAnalysis analysis = {};

  // Determine workload status
  if (smoothGpu < 5.0 && smoothCpu < 15.0) {
    analysis.status = "Idle";
    analysis.bottleneck = "Idle";
    analysis.estimatedFPS = 0;
    analysis.gpuBound = false;
    analysis.cpuBound = false;
  } else if (smoothGpu < 30.0 && smoothCpu < 40.0) {
    analysis.status = "Light Load";
    analysis.bottleneck = "Balanced";
    analysis.gpuBound = false;
    analysis.cpuBound = false;
  } else {
    analysis.status = "Gaming";

    // Bottleneck detection
    if (smoothGpu > 85.0 && smoothCpu < 60.0) {
      analysis.gpuBound = true;
      analysis.cpuBound = false;
      analysis.bottleneck = "GPU Bound";
    } else if (smoothCpu > 85.0 && smoothGpu < 60.0) {
      analysis.gpuBound = false;
      analysis.cpuBound = true;
      analysis.bottleneck = "CPU Bound";
    } else if (smoothGpu > 80.0 && smoothCpu > 80.0) {
      analysis.gpuBound = true;
      analysis.cpuBound = true;
      analysis.bottleneck = "Both Limited";
    } else {
      analysis.gpuBound = false;
      analysis.cpuBound = false;
      analysis.bottleneck = "Balanced";
    }
  }

  // Estimate frame times (rough estimation based on utilization and clock)
  if (gpuMaxClockMHz > 0 && gpuClockMHz > 0 && smoothGpu > 5.0) {
    // GPU frame time estimation based on clock ratio and utilization
    double clockRatio = gpuClockMHz / gpuMaxClockMHz;
    // At 100% GPU util, frame time depends on the workload
    // Rough model: frame time ≈ base_time / (clock_ratio * (1 - headroom))
    double gpuHeadroom = (100.0 - smoothGpu) / 100.0;
    if (gpuHeadroom < 0.01)
      gpuHeadroom = 0.01;

    // Assume optimal frame time at current clock is ~8.3ms (120fps target)
    analysis.gpuFrameTimeMs = 8.33 / (clockRatio * (1.0 - smoothGpu / 200.0));
    if (analysis.gpuFrameTimeMs > 100.0)
      analysis.gpuFrameTimeMs = 100.0;
    if (analysis.gpuFrameTimeMs < 1.0)
      analysis.gpuFrameTimeMs = 1.0;
  } else {
    analysis.gpuFrameTimeMs = 0.0;
  }

  // CPU frame time estimation
  if (smoothCpu > 5.0) {
    // Simple model: higher CPU usage → longer frame time
    analysis.cpuFrameTimeMs = 4.0 + (smoothCpu / 100.0) * 12.0;
  } else {
    analysis.cpuFrameTimeMs = 0.0;
  }

  // Estimate FPS from the larger frame time (bottleneck)
  if (analysis.gpuFrameTimeMs > 0 || analysis.cpuFrameTimeMs > 0) {
    double maxFrameTime =
        std::max(analysis.gpuFrameTimeMs, analysis.cpuFrameTimeMs);
    if (maxFrameTime > 0) {
      analysis.estimatedFPS = 1000.0 / maxFrameTime;
      if (analysis.estimatedFPS > 999.0)
        analysis.estimatedFPS = 999.0;
    }
  }

  cachedAnalysis = analysis;
}

FrameAnalysis PerformanceAnalyzer::getAnalysis() const {
  return cachedAnalysis;
}
