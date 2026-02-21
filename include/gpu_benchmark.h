#pragma once
#include "nvml_monitor.h"
#include <string>
#include <vector>

// ============================================================================
// Benchmark Result
// ============================================================================

struct BenchmarkResult {
  // Theoretical Performance
  float fp32Tflops = 0.0f;
  float fp16Tflops = 0.0f;
  float int8Tops = 0.0f;

  // Measured Bandwidth
  float vramBandwidthGBs = 0.0f;
  bool bandwidthMeasured = false;

  // Score
  int score = 0; // 0-100 AI readiness score
  std::string scoreLabel;
  std::string timestamp;

  // GPU Info used for calculation
  int cudaCores = 0;
  int smCount = 0;
  unsigned int clockMHz = 0;
  bool hasTensorCores = false;

  bool valid = false;
};

// ============================================================================
// GPU Benchmark Class
// ============================================================================

class GpuBenchmark {
public:
  GpuBenchmark() = default;

  // Run benchmark using NVML data + DXGI bandwidth test
  BenchmarkResult runBenchmark(const NvmlInfo &nvmlInfo);

  // Access results
  const BenchmarkResult &getLastResult() const { return lastResult; }
  const std::vector<BenchmarkResult> &getHistory() const { return history; }
  bool isRunning() const { return running; }

private:
  int estimateSmCount(const NvmlInfo &nvmlInfo) const;
  int getCoresPerSm(int major, int minor) const;
  float measureVramBandwidth() const;
  int calculateScore(const BenchmarkResult &result) const;
  std::string generateScoreLabel(int score) const;
  std::string getCurrentTimestamp() const;

  BenchmarkResult lastResult;
  std::vector<BenchmarkResult> history;
  bool running = false;
};
