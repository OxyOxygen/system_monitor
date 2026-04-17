#pragma once

#include <string>
#include <vector>
#include "nvml_monitor.h"
#include "network_monitor.h"
#include "disk_monitor.h"
#include "disk_io_monitor.h"
#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "gaming_session.h"

struct HpcInsight {
    enum class Level { Info, Warning, Critical };
    Level level = Level::Info;
    std::string title;
    std::string description;
    std::string recommendation;
};

struct HpcReport {
    int performanceScore; // 0-100
    std::vector<HpcInsight> insights;
    bool isBottlenecked;
    std::string primaryBottleneck;
};

class HpcEngine {
public:
    HpcEngine();
    ~HpcEngine() = default;

    HpcReport analyze(const NvmlInfo& nvml, 
                      const NetworkInfo& net,
                      const DiskIOInfo& disk,
                      double cpuUsage,
                      const MemoryInfo& mem);

    HpcReport generateSessionReport(const SessionMetrics& metrics);

private:
    enum class FrameworkType { None, PyTorch, TensorFlow };
    struct AiProcess {
        unsigned int pid;
        std::string name;
        FrameworkType type;
    };

    FrameworkType detectFramework(const std::string& processName);
    void checkFrameworkHeuristics(const NvmlInfo& nvml, double cpuUsage, HpcReport& report);
    
    void checkGpuBottlenecks(const NvmlInfo& nvml, HpcReport& report);
    void checkCpuMemBottlenecks(double cpuUsage, const MemoryInfo& mem, HpcReport& report);
    void checkStorageBottlenecks(const DiskIOInfo& disk, HpcReport& report);
    void checkNetworkBottlenecks(const NetworkInfo& net, HpcReport& report);

    void addInsight(HpcReport& report, HpcInsight::Level level, 
                    const std::string& title, const std::string& desc, 
                    const std::string& rec);
};
