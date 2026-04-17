#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include "nvml_monitor.h"
#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "network_monitor.h"
#include "process_monitor.h"

struct SessionMetrics {
    // Timestamps
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    double durationSeconds;

    // Averages and Peaks
    double avgCpuUsage, maxCpuUsage;
    double avgGpuUsage, maxGpuUsage;
    double avgVramUsageGB, maxVramUsageGB;
    double avgRamUsageGB, maxRamUsageGB;
    double avgCpuTemp, maxCpuTemp;
    double avgGpuTemp, maxGpuTemp;
    double avgPing, maxPing;
    
    // FPS Metrics
    double avgFps, maxFps, minFps;
    double gpuAvgPower;

    // History for graphs (entire session)
    std::vector<float> fpsHistory;
    std::vector<float> cpuHistory;
    std::vector<float> gpuHistory;
    std::vector<float> vramHistory;
    std::vector<float> pingHistory;

    // Top Processes during session
    std::vector<ProcessInfo> topProcesses;
};

class GamingSessionMonitor {
public:
    GamingSessionMonitor();
    ~GamingSessionMonitor() = default;

    void startSession();
    void stopSession();
    void update(double cpuUsage, const NvmlInfo& nvml, const MemoryInfo& mem, const NetworkInfo& net, const std::vector<ProcessInfo>& currentProcesses, double fps);

    bool isActive() const { return active; }
    const SessionMetrics& getLastSessionMetrics() const { return lastMetrics; }
    double getCurrentPing() const { return currentPing; }

private:
    bool active;
    SessionMetrics currentMetrics;
    SessionMetrics lastMetrics;

    // Accumulators for averaging
    uint64_t tickCount;
    double sumCpu, sumGpu, sumVram, sumRam, sumCpuTemp, sumGpuTemp, sumPing, sumFps, sumGpuPower;
    double currentPing;

    // Sampling control
    std::chrono::steady_clock::time_point lastSampleTime;
};
