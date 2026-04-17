#include "gaming_session.h"
#include <utility>

GamingSessionMonitor::GamingSessionMonitor() : active(false), tickCount(0), currentPing(0) {
    currentMetrics = {};
    lastMetrics = {};
}

void GamingSessionMonitor::startSession() {
    active = true;
    tickCount = 0;
    sumCpu = sumGpu = sumVram = sumRam = sumCpuTemp = sumGpuTemp = sumPing = sumFps = sumGpuPower = 0;
    currentPing = 0;
    
    currentMetrics = {};
    currentMetrics.startTime = std::chrono::steady_clock::now();
    currentMetrics.maxCpuUsage = currentMetrics.maxGpuUsage = 0;
    currentMetrics.maxVramUsageGB = currentMetrics.maxRamUsageGB = 0;
    currentMetrics.maxCpuTemp = currentMetrics.maxGpuTemp = 0;
    currentMetrics.maxPing = 0;
    currentMetrics.maxFps = 0;
    currentMetrics.minFps = 9999.0;
    
    currentMetrics.fpsHistory.clear();
    currentMetrics.cpuHistory.clear();
    currentMetrics.gpuHistory.clear();
    currentMetrics.vramHistory.clear();
    currentMetrics.pingHistory.clear();
}

void GamingSessionMonitor::stopSession() {
    if (!active) return;
    
    active = false;
    currentMetrics.endTime = std::chrono::steady_clock::now();
    currentMetrics.durationSeconds = std::chrono::duration<double>(currentMetrics.endTime - currentMetrics.startTime).count();

    if (tickCount > 0) {
        currentMetrics.avgCpuUsage = sumCpu / tickCount;
        currentMetrics.avgGpuUsage = sumGpu / tickCount;
        currentMetrics.avgVramUsageGB = sumVram / tickCount;
        currentMetrics.avgRamUsageGB = sumRam / tickCount;
        currentMetrics.avgCpuTemp = sumCpuTemp / tickCount;
        currentMetrics.avgGpuTemp = sumGpuTemp / tickCount;
        currentMetrics.avgPing = sumPing / tickCount;
        currentMetrics.avgFps = sumFps / tickCount;
        currentMetrics.gpuAvgPower = sumGpuPower / tickCount;
    }
    
    if (currentMetrics.minFps == 9999.0) currentMetrics.minFps = 0;

    lastMetrics = currentMetrics;
}

void GamingSessionMonitor::update(double cpuUsage, const NvmlInfo& nvml, const MemoryInfo& mem, const NetworkInfo& net, const std::vector<ProcessInfo>& currentProcesses, double fps) {
    if (!active) return;

    tickCount++;
    
    // CPU
    sumCpu += cpuUsage;
    if (cpuUsage > currentMetrics.maxCpuUsage) currentMetrics.maxCpuUsage = cpuUsage;
    currentMetrics.cpuHistory.push_back(static_cast<float>(cpuUsage));

    // FPS
    sumFps += fps;
    if (fps > currentMetrics.maxFps) currentMetrics.maxFps = fps;
    if (fps > 0 && fps < currentMetrics.minFps) currentMetrics.minFps = fps;
    currentMetrics.fpsHistory.push_back(static_cast<float>(fps));

    // GPU (First device primarily)
    if (!nvml.devices.empty()) {
        double gpuU = nvml.devices[0].gpuUtil;
        double vramU = nvml.devices[0].vramUsed / (1024.0 * 1024.0 * 1024.0);
        double gpuT = nvml.devices[0].temperatureC;
        double gpuP = nvml.devices[0].powerWatts;

        sumGpu += gpuU;
        sumVram += vramU;
        sumGpuTemp += gpuT;
        sumGpuPower += gpuP;

        if (gpuU > currentMetrics.maxGpuUsage) currentMetrics.maxGpuUsage = gpuU;
        if (vramU > currentMetrics.maxVramUsageGB) currentMetrics.maxVramUsageGB = vramU;
        if (gpuT > currentMetrics.maxGpuTemp) currentMetrics.maxGpuTemp = gpuT;
        
        currentMetrics.gpuHistory.push_back(static_cast<float>(gpuU));
        currentMetrics.vramHistory.push_back(static_cast<float>(vramU));
    }

    // RAM
    double ramU = mem.usedRAM / (1024.0 * 1024.0 * 1024.0);
    sumRam += ramU;
    if (ramU > currentMetrics.maxRamUsageGB) currentMetrics.maxRamUsageGB = ramU;

    // Network Ping
    double ping = net.pingLatencyMs;
    currentPing = ping;
    if (ping >= 0) {
        sumPing += ping;
        if (ping > currentMetrics.maxPing) currentMetrics.maxPing = ping;
        currentMetrics.pingHistory.push_back(static_cast<float>(ping));
    }

    // Top Processes during session
    if (currentMetrics.topProcesses.empty() && !currentProcesses.empty()) {
        for (int i = 0; i < (std::min)(3, (int)currentProcesses.size()); i++) {
            currentMetrics.topProcesses.push_back(currentProcesses[i]);
        }
    }

    // Limit history vectors to 600 samples (~10 mins at 1sec) to prevent memory issues
    // Sub-sampling could be done here if session gets very long
    if (currentMetrics.fpsHistory.size() > 600) {
        // Simple sub-sampling: remove every 2nd element
        auto subSample = [](std::vector<float>& vec) {
            std::vector<float> next;
            for (size_t i = 0; i < vec.size(); i += 2) next.push_back(vec[i]);
            vec = std::move(next);
        };
        subSample(currentMetrics.fpsHistory);
        subSample(currentMetrics.cpuHistory);
        subSample(currentMetrics.gpuHistory);
        subSample(currentMetrics.vramHistory);
        subSample(currentMetrics.pingHistory);
    }
}
