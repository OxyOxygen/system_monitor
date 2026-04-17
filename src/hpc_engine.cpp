#include "hpc_engine.h"
#include <algorithm>
#include <cctype>
#include <string>

HpcEngine::HpcEngine() {}

HpcReport HpcEngine::analyze(const NvmlInfo& nvml, 
                             const NetworkInfo& net,
                             const DiskIOInfo& disk,
                             double cpuUsage,
                             const MemoryInfo& mem) {
    HpcReport report = {};
    report.performanceScore = 100;
    report.isBottlenecked = false;
    report.primaryBottleneck = "None";

    checkGpuBottlenecks(nvml, report);
    checkCpuMemBottlenecks(cpuUsage, mem, report);
    checkFrameworkHeuristics(nvml, cpuUsage, report);
    checkStorageBottlenecks(disk, report);
    checkNetworkBottlenecks(net, report);

    // Final score adjustment based on warnings
    int penalties = 0;
    for (const auto& insight : report.insights) {
        if (insight.level == HpcInsight::Level::Critical) penalties += 25;
        else if (insight.level == HpcInsight::Level::Warning) penalties += 10;
    }
    report.performanceScore = (std::max)(0, 100 - penalties);

    return report;
}

void HpcEngine::checkGpuBottlenecks(const NvmlInfo& nvml, HpcReport& report) {
    if (!nvml.available || nvml.devices.empty()) return;

    for (size_t i = 0; i < nvml.devices.size(); i++) {
        const auto& dev = nvml.devices[i];
        std::string gpuPrefix = "GPU #" + std::to_string(i) + " (" + dev.name + "): ";

        // VRAM Pressure (AI Critical)
        double vramUsage = (dev.vramTotal > 0) ? (double)dev.vramUsed / dev.vramTotal : 0;
        if (vramUsage > 0.95) {
            addInsight(report, HpcInsight::Level::Critical, "VRAM Saturation",
                       gpuPrefix + "VRAM is almost full (" + std::to_string((int)(vramUsage * 100)) + "%).",
                       "Reduce batch size or use gradient checkpointing to avoid Out-Of-Memory errors.");
            report.isBottlenecked = true;
            report.primaryBottleneck = "GPU VRAM";
        } else if (vramUsage > 0.80) {
            addInsight(report, HpcInsight::Level::Warning, "High VRAM Usage",
                       gpuPrefix + "VRAM usage is high (" + std::to_string((int)(vramUsage * 100)) + "%).",
                       "Monitor for potential OOM if the model scales or context window increases.");
        }

        // PCIe Bandwidth (Data Transfer Bottleneck)
        // Heuristic: High PCIe throughput with moderate GPU utilization suggests data bottleneck
        if (dev.pcieRxKBs > 8000000) { // > ~8GB/s
             addInsight(report, HpcInsight::Level::Warning, "High PCIe Bandwidth",
                       gpuPrefix + "PCIe bus is heavily loaded with data transfers.",
                       "Consider data pre-fetching or pinning memory to improve transfer speeds.");
        }

        // Thermal Throttling
        if (dev.temperatureC > 85) {
            addInsight(report, HpcInsight::Level::Critical, "Thermal Throttling",
                       gpuPrefix + "Temperature is critical (" + std::to_string(dev.temperatureC) + "C).",
                       "Hardware is likely downclocking. Check cooling or reduce ambient temperature.");
            report.isBottlenecked = true;
            report.primaryBottleneck = "Thermal";
        } else if (dev.temperatureC > 75) {
            addInsight(report, HpcInsight::Level::Warning, "High Temperature",
                       gpuPrefix + "GPU is running hot.",
                       "Ensure proper airflow to maintain peak clock speeds.");
        }

        // Compute vs Memory Interface
        if (dev.gpuUtil > 90 && dev.memUtil < 30) {
            addInsight(report, HpcInsight::Level::Info, "Compute Bound",
                       gpuPrefix + "GPU is compute-limited.",
                       "Your model is mathematically intensive. Faster CUDA cores would benefit this workload.");
        } else if (dev.gpuUtil < 50 && dev.memUtil > 80) {
            addInsight(report, HpcInsight::Level::Warning, "Memory Interface Bound",
                       gpuPrefix + "GPU is stalled waiting for memory operations.",
                       "Consider optimizing memory access patterns or using a GPU with higher memory bandwidth.");
            report.isBottlenecked = true;
            report.primaryBottleneck = "GPU Memory BW";
        }
    }
}

void HpcEngine::checkCpuMemBottlenecks(double cpuUsage, const MemoryInfo& mem, HpcReport& report) {
    if (cpuUsage > 95) {
        addInsight(report, HpcInsight::Level::Critical, "CPU Saturation",
                   "CPU is fully utilized, causing potential stalls in data preprocessing.",
                   "Offload preprocessing to GPU (if supported) or increase CPU core count.");
        report.isBottlenecked = true;
        if (report.primaryBottleneck == "None") report.primaryBottleneck = "CPU Compute";
    }

    double memUsage = mem.usagePercent;
    if (memUsage > 90) {
        addInsight(report, HpcInsight::Level::Critical, "System RAM Critical",
                   "System memory is almost exhausted. Risk of disk swapping.",
                   "Close background applications or increase system RAM to avoid significant slowdowns.");
        report.isBottlenecked = true;
        report.primaryBottleneck = "System RAM";
    }
}

void HpcEngine::checkStorageBottlenecks(const DiskIOInfo& disk, HpcReport& report) {
    if (!disk.available) return;

    // High read latency or high utilization
    if (disk.readBytesPerSec > 500 * 1024 * 1024) { // > 500MB/s 
        // This is high for some SATA SSDs, but fine for NVMe. 
        // A better check would be IO latency if available.
    }
}

void HpcEngine::checkNetworkBottlenecks(const NetworkInfo& net, HpcReport& report) {
    if (net.pingLatencyMs > 150) {
        addInsight(report, HpcInsight::Level::Warning, "High Network Latency",
                   "Network latency is high (" + std::to_string((int)net.pingLatencyMs) + "ms).",
                   "Expect delays in distributed training Synchronization or remote data fetching.");
    }
}

void HpcEngine::checkFrameworkHeuristics(const NvmlInfo& nvml, double cpuUsage, HpcReport& report) {
    if (nvml.devices.empty()) return;

    for (const auto& dev : nvml.devices) {
        for (const auto& proc : dev.gpuProcesses) {
            FrameworkType type = detectFramework(proc.name);
            if (type == FrameworkType::None) continue;

            std::string frameworkName = (type == FrameworkType::PyTorch) ? "PyTorch" : "TensorFlow";
            std::string procPrefix = "[" + frameworkName + "] Process (" + proc.name + "): ";

            // 1. Data Loading Bottleneck (Common for both)
            if (dev.gpuUtil < 40 && cpuUsage > 70) {
                addInsight(report, HpcInsight::Level::Warning, frameworkName + " Data Stall",
                           procPrefix + "High CPU usage with low GPU utilization suggests a data loading bottleneck.",
                           (type == FrameworkType::PyTorch) 
                           ? "Increase 'num_workers' in your DataLoader and ensure 'pin_memory=True' is set."
                           : "Optimize your 'tf.data' pipeline with '.prefetch(tf.data.AUTOTUNE)' and '.cache()'.");
            }

            // 2. Framework Specifics
            if (type == FrameworkType::PyTorch) {
                // Fragmented VRAM / Allocation issues
                if (dev.vramUsed > dev.vramTotal * 0.9 && dev.gpuUtil < 20) {
                    addInsight(report, HpcInsight::Level::Info, "PyTorch Memory Fragmentation",
                               procPrefix + "High VRAM usage but low compute may indicate memory fragmentation.",
                               "Consider setting 'PYTORCH_CUDA_ALLOC_CONF=max_split_size_mb:128' to reduce fragmentation.");
                }
            } else if (type == FrameworkType::TensorFlow) {
                // VRAM Growth (Detect if whole VRAM is taken immediately)
                if (dev.vramUsed > dev.vramTotal * 0.95 && dev.gpuUtil < 10) {
                    addInsight(report, HpcInsight::Level::Warning, "TensorFlow VRAM Hogging",
                               procPrefix + "TensorFlow has allocated almost all VRAM while idle.",
                               "Use 'tf.config.experimental.set_memory_growth(gpu, True)' to allocate memory as needed.");
                }
            }
        }
    }
}

HpcEngine::FrameworkType HpcEngine::detectFramework(const std::string& processName) {
    std::string lowerName = processName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    if (lowerName.find("torch") != std::string::npos || lowerName.find("python") != std::string::npos) {
        // Since many AI apps run as python.exe, we lean towards PyTorch as default if it's an AI task
        // but we can refine this if we see "tensorflow" or "tensorboard"
        if (lowerName.find("tensorflow") != std::string::npos || lowerName.find("tfevent") != std::string::npos) {
            return FrameworkType::TensorFlow;
        }
        // Most common AI framework on Windows for researchers is PyTorch
        return FrameworkType::PyTorch;
    }
    return FrameworkType::None;
}

void HpcEngine::addInsight(HpcReport& report, HpcInsight::Level level, 
                           const std::string& title, const std::string& desc, 
                           const std::string& rec) {
    HpcInsight insight;
    insight.level = level;
    insight.title = title;
    insight.description = desc;
    insight.recommendation = rec;
    report.insights.push_back(insight);
}

HpcReport HpcEngine::generateSessionReport(const SessionMetrics& metrics) {
    HpcReport report = {};
    report.performanceScore = 100;
    report.isBottlenecked = false;

    // 1. Session Summary
    addInsight(report, HpcInsight::Level::Info, "Session Completed", 
               "Total session duration: " + std::to_string(static_cast<int>(metrics.durationSeconds / 60)) + " minutes.", 
               "Performance data successfully analyzed.");

    // 2. FPS Analysis
    if (metrics.avgFps > 0) {
        std::string fpsDesc = "Average: " + std::to_string(static_cast<int>(metrics.avgFps)) + " FPS. " +
                               "Minimum: " + std::to_string(static_cast<int>(metrics.minFps)) + " FPS.";
        std::string rec = "Stability looks good.";
        if (metrics.minFps < metrics.avgFps * 0.5) {
            rec = "Detected high frame variance. Consider lowering shadow quality to stabilize FPS.";
        }
        addInsight(report, HpcInsight::Level::Info, "Frame Rate Performance", fpsDesc, rec);
    }

    // 3. GPU Analysis
    if (metrics.maxGpuUsage > 98.0) {
        addInsight(report, HpcInsight::Level::Warning, "GPU Utilization Peak",
                   "Your GPU was fully saturated with an average load of " + std::to_string(static_cast<int>(metrics.avgGpuUsage)) + "%.",
                   "Lowering Ray Tracing or Resolution Scale will provide smoother gameplay.");
        report.isBottlenecked = true;
        report.primaryBottleneck = "GPU";
    }

    // 4. Thermal Analysis
    if (metrics.maxGpuTemp > 82.0) {
        addInsight(report, HpcInsight::Level::Critical, "High GPU Temperature",
                   "GPU temperature peaked at " + std::to_string(static_cast<int>(metrics.maxGpuTemp)) + "°C during intense scenes.",
                   "Check case airflow or increase GPU fan curve speed.");
    }
    
    if (metrics.maxCpuTemp > 85.0) {
        addInsight(report, HpcInsight::Level::Critical, "Heavy CPU Thermal Load",
                   "CPU temperature reached " + std::to_string(static_cast<int>(metrics.maxCpuTemp)) + "°C.",
                   "Ensure CPU cooler is properly seated and thermal paste is fresh.");
    }

    // 5. Memory & VRAM
    if (metrics.maxVramUsageGB > 8.0) {
        addInsight(report, HpcInsight::Level::Info, "VRAM Saturation",
                   "Used " + std::to_string(static_cast<int>(metrics.maxVramUsageGB)) + "GB of VRAM.",
                   "Reducing Texture Quality can prevent stuttering in open-world areas.");
    }

    // 6. Running Processes
    if (!metrics.topProcesses.empty()) {
        std::string procDesc = "Active background processes: ";
        for (const auto& p : metrics.topProcesses) {
            procDesc += p.name + " ";
        }
        addInsight(report, HpcInsight::Level::Info, "Background Activity",
                   procDesc, "Closing unnecessary apps can reduce CPU frame time latency.");
    }

    return report;
}
