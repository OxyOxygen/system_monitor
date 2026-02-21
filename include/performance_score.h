#pragma once

#include <cstdint>
#include <string>

struct ScoreInfo {
  int overallScore;  // 0-100
  int cpuScore;      // 0-100
  int gpuScore;      // 0-100
  int memoryScore;   // 0-100
  int diskScore;     // 0-100
  std::string grade; // S, A, B, C, D, F
};

class PerformanceScore {
public:
  PerformanceScore();
  ~PerformanceScore() = default;

  void update(int cpuCores, double cpuClockGHz, uint64_t totalRAM,
              uint64_t availableRAM, uint64_t vramTotal, double tflops,
              int cudaComputeCapMajor, uint64_t diskTotal, uint64_t diskFree);

  ScoreInfo getScore() const;

private:
  ScoreInfo cachedScore;
  bool hasCalculated;

  static std::string gradeFromScore(int score);
};
