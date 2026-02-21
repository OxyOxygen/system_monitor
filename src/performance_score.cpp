#include "performance_score.h"
#include <algorithm>

PerformanceScore::PerformanceScore() : hasCalculated(false) {
  cachedScore = {};
  cachedScore.grade = "?";
}

std::string PerformanceScore::gradeFromScore(int score) {
  if (score >= 90)
    return "S";
  if (score >= 80)
    return "A";
  if (score >= 65)
    return "B";
  if (score >= 50)
    return "C";
  if (score >= 35)
    return "D";
  return "F";
}

void PerformanceScore::update(int cpuCores, double cpuClockGHz,
                              uint64_t totalRAM, uint64_t availableRAM,
                              uint64_t vramTotal, double tflops,
                              int cudaComputeCapMajor, uint64_t diskTotal,
                              uint64_t diskFree) {
  // ── CPU Score (0-100) ──
  // Based on core count and approximate performance tier
  int cpuScore = 0;
  if (cpuCores >= 24)
    cpuScore = 95;
  else if (cpuCores >= 16)
    cpuScore = 85;
  else if (cpuCores >= 12)
    cpuScore = 75;
  else if (cpuCores >= 8)
    cpuScore = 65;
  else if (cpuCores >= 6)
    cpuScore = 50;
  else if (cpuCores >= 4)
    cpuScore = 35;
  else
    cpuScore = 20;

  // Clock speed bonus
  if (cpuClockGHz >= 5.0)
    cpuScore = std::min(100, cpuScore + 10);
  else if (cpuClockGHz >= 4.0)
    cpuScore = std::min(100, cpuScore + 5);

  // ── GPU Score (0-100) ──
  int gpuScore = 0;
  double vramGB = static_cast<double>(vramTotal) / (1024.0 * 1024.0 * 1024.0);

  if (vramGB >= 24)
    gpuScore = 95;
  else if (vramGB >= 16)
    gpuScore = 85;
  else if (vramGB >= 12)
    gpuScore = 75;
  else if (vramGB >= 8)
    gpuScore = 60;
  else if (vramGB >= 6)
    gpuScore = 45;
  else if (vramGB >= 4)
    gpuScore = 35;
  else if (vramGB >= 2)
    gpuScore = 20;
  else
    gpuScore = 10;

  // TFLOPS bonus
  if (tflops >= 30.0)
    gpuScore = std::min(100, gpuScore + 10);
  else if (tflops >= 15.0)
    gpuScore = std::min(100, gpuScore + 5);

  // Compute capability bonus
  if (cudaComputeCapMajor >= 8)
    gpuScore = std::min(100, gpuScore + 5);

  // If no GPU detected
  if (vramTotal == 0)
    gpuScore = 0;

  // ── Memory Score (0-100) ──
  int memoryScore = 0;
  double totalGB = static_cast<double>(totalRAM) / (1024.0 * 1024.0 * 1024.0);

  if (totalGB >= 128)
    memoryScore = 100;
  else if (totalGB >= 64)
    memoryScore = 90;
  else if (totalGB >= 32)
    memoryScore = 80;
  else if (totalGB >= 16)
    memoryScore = 60;
  else if (totalGB >= 8)
    memoryScore = 40;
  else
    memoryScore = 20;

  // Available memory penalty
  double availPercent = (totalRAM > 0)
                            ? static_cast<double>(availableRAM) /
                                  static_cast<double>(totalRAM) * 100.0
                            : 0;
  if (availPercent < 10.0)
    memoryScore = std::max(0, memoryScore - 15);
  else if (availPercent < 20.0)
    memoryScore = std::max(0, memoryScore - 5);

  // ── Disk Score (0-100) ──
  int diskScore = 0;
  double diskTotalGB =
      static_cast<double>(diskTotal) / (1024.0 * 1024.0 * 1024.0);

  if (diskTotalGB >= 2000)
    diskScore = 90;
  else if (diskTotalGB >= 1000)
    diskScore = 80;
  else if (diskTotalGB >= 500)
    diskScore = 65;
  else if (diskTotalGB >= 256)
    diskScore = 50;
  else
    diskScore = 30;

  // Free space bonus/penalty
  double freePercent = (diskTotal > 0)
                           ? static_cast<double>(diskFree) /
                                 static_cast<double>(diskTotal) * 100.0
                           : 0;
  if (freePercent > 50.0)
    diskScore = std::min(100, diskScore + 10);
  else if (freePercent < 10.0)
    diskScore = std::max(0, diskScore - 15);

  // ── Overall Score ──
  // Weighted: CPU 30%, GPU 35%, RAM 20%, Disk 15%
  int overall = static_cast<int>(cpuScore * 0.30 + gpuScore * 0.35 +
                                 memoryScore * 0.20 + diskScore * 0.15);
  overall = std::clamp(overall, 0, 100);

  cachedScore.cpuScore = cpuScore;
  cachedScore.gpuScore = gpuScore;
  cachedScore.memoryScore = memoryScore;
  cachedScore.diskScore = diskScore;
  cachedScore.overallScore = overall;
  cachedScore.grade = gradeFromScore(overall);

  hasCalculated = true;
}

ScoreInfo PerformanceScore::getScore() const { return cachedScore; }
