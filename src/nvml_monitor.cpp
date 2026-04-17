#include "nvml_monitor.h"
#include <algorithm>
#include <filesystem>
#include <psapi.h>
#include <tlhelp32.h>
#include <vector>

std::string NvmlMonitor::resolveProcessNameWithCache(unsigned int pid) {
  auto now = std::chrono::steady_clock::now();
  auto it = processNameCache.find(pid);
  if (it != processNameCache.end() && (now - it->second.lastUpdated) < std::chrono::seconds(5)) {
    return it->second.name;
  }

  std::string name = "Unknown";
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (hProcess) {
    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
      std::filesystem::path p(path);
      std::wstring filename = p.filename().wstring();
      int required_size = WideCharToMultiByte(CP_UTF8, 0, filename.c_str(), -1, nullptr, 0, nullptr, nullptr);
      if (required_size > 0) {
        std::vector<char> buffer(required_size);
        WideCharToMultiByte(CP_UTF8, 0, filename.c_str(), -1, buffer.data(), required_size, nullptr, nullptr);
        name = std::string(buffer.data());
      }
    }
    CloseHandle(hProcess);
  }

  processNameCache[pid] = {name, now};
  return name;
}

// ============================================================================
// Library Loading
// ============================================================================

bool NvmlMonitor::loadLibrary() {
  // Try common NVML DLL locations
  const char *paths[] = {
      "nvml.dll", "C:\\Program Files\\NVIDIA Corporation\\NVSMI\\nvml.dll",
      "C:\\Windows\\System32\\nvml.dll"};

  for (const auto &path : paths) {
    hNvml = LoadLibraryA(path);
    if (hNvml)
      return true;
  }
  return false;
}

void NvmlMonitor::resolveSymbols() {
  if (!hNvml)
    return;

#define LOAD_FN(name, type)                                                    \
  name = reinterpret_cast<type>(GetProcAddress(hNvml, #type + 4))

  // Manual resolution — NVML exported names differ from typedefs
  pfnInit =
      reinterpret_cast<PFN_nvmlInit_v2>(GetProcAddress(hNvml, "nvmlInit_v2"));
  pfnShutdown =
      reinterpret_cast<PFN_nvmlShutdown>(GetProcAddress(hNvml, "nvmlShutdown"));
  pfnGetCount = reinterpret_cast<PFN_nvmlDeviceGetCount_v2>(
      GetProcAddress(hNvml, "nvmlDeviceGetCount_v2"));
  pfnGetHandle = reinterpret_cast<PFN_nvmlDeviceGetHandleByIndex_v2>(
      GetProcAddress(hNvml, "nvmlDeviceGetHandleByIndex_v2"));
  pfnGetTemp = reinterpret_cast<PFN_nvmlDeviceGetTemperature>(
      GetProcAddress(hNvml, "nvmlDeviceGetTemperature"));
  pfnGetPower = reinterpret_cast<PFN_nvmlDeviceGetPowerUsage>(
      GetProcAddress(hNvml, "nvmlDeviceGetPowerUsage"));
  pfnGetUtil = reinterpret_cast<PFN_nvmlDeviceGetUtilizationRates>(
      GetProcAddress(hNvml, "nvmlDeviceGetUtilizationRates"));
  pfnGetClock = reinterpret_cast<PFN_nvmlDeviceGetClockInfo>(
      GetProcAddress(hNvml, "nvmlDeviceGetClockInfo"));
  pfnGetMaxClock = reinterpret_cast<PFN_nvmlDeviceGetMaxClockInfo>(
      GetProcAddress(hNvml, "nvmlDeviceGetMaxClockInfo"));
  pfnGetFan = reinterpret_cast<PFN_nvmlDeviceGetFanSpeed>(
      GetProcAddress(hNvml, "nvmlDeviceGetFanSpeed"));
  pfnGetPcie = reinterpret_cast<PFN_nvmlDeviceGetPcieThroughput>(
      GetProcAddress(hNvml, "nvmlDeviceGetPcieThroughput"));
  pfnGetEncoder = reinterpret_cast<PFN_nvmlDeviceGetEncoderUtilization>(
      GetProcAddress(hNvml, "nvmlDeviceGetEncoderUtilization"));
  pfnGetDecoder = reinterpret_cast<PFN_nvmlDeviceGetDecoderUtilization>(
      GetProcAddress(hNvml, "nvmlDeviceGetDecoderUtilization"));
  pfnGetCudaCap = reinterpret_cast<PFN_nvmlDeviceGetCudaComputeCapability>(
      GetProcAddress(hNvml, "nvmlDeviceGetCudaComputeCapability"));
  pfnGetDriverVer = reinterpret_cast<PFN_nvmlSystemGetDriverVersion>(
      GetProcAddress(hNvml, "nvmlSystemGetDriverVersion"));
  pfnGetProcesses =
      reinterpret_cast<PFN_nvmlDeviceGetComputeRunningProcesses_v3>(
          GetProcAddress(hNvml, "nvmlDeviceGetComputeRunningProcesses_v3"));
  pfnGetName = reinterpret_cast<PFN_nvmlDeviceGetName>(
      GetProcAddress(hNvml, "nvmlDeviceGetName"));
  pfnGetMemInfo = reinterpret_cast<PFN_nvmlDeviceGetMemoryInfo>(
      GetProcAddress(hNvml, "nvmlDeviceGetMemoryInfo"));
  pfnGetEnforcedPowerLimit = reinterpret_cast<PFN_nvmlDeviceGetEnforcedPowerLimit>(
      GetProcAddress(hNvml, "nvmlDeviceGetEnforcedPowerLimit"));
  pfnGetMaxPcieLinkGen = reinterpret_cast<PFN_nvmlDeviceGetMaxPcieLinkGeneration>(
      GetProcAddress(hNvml, "nvmlDeviceGetMaxPcieLinkGeneration"));
  pfnGetCurrPcieLinkGen = reinterpret_cast<PFN_nvmlDeviceGetCurrPcieLinkGeneration>(
      GetProcAddress(hNvml, "nvmlDeviceGetCurrPcieLinkGeneration"));
  pfnGetCurrPcieLinkWidth = reinterpret_cast<PFN_nvmlDeviceGetCurrPcieLinkWidth>(
      GetProcAddress(hNvml, "nvmlDeviceGetCurrPcieLinkWidth"));

#undef LOAD_FN
}

// ============================================================================
// Init & Shutdown
// ============================================================================

bool NvmlMonitor::init() {
  if (!loadLibrary())
    return false;

  resolveSymbols();

  // Must have at least init, shutdown, and device handle
  if (!pfnInit || !pfnShutdown || !pfnGetHandle) {
    FreeLibrary(hNvml);
    hNvml = nullptr;
    return false;
  }

  if (pfnInit() != NVML_SUCCESS) {
    FreeLibrary(hNvml);
    hNvml = nullptr;
    return false;
  }

  // Get device handles for all GPUs
  unsigned int deviceCount = 0;
  if (pfnGetCount)
    pfnGetCount(&deviceCount);

  if (deviceCount == 0) {
    pfnShutdown();
    FreeLibrary(hNvml);
    hNvml = nullptr;
    return false;
  }

  devices.clear();
  info.devices.clear();

  for (unsigned int i = 0; i < deviceCount; i++) {
    nvmlDevice_t dev = nullptr;
    if (pfnGetHandle && pfnGetHandle(i, &dev) == NVML_SUCCESS) {
      devices.push_back(dev);
      NvmlDeviceInfo devInfo = {};
      
      // Static info for this device
      if (pfnGetName) {
        char name[128] = {};
        if (pfnGetName(dev, name, sizeof(name)) == NVML_SUCCESS)
          devInfo.name = name;
      }
      
      if (pfnGetCudaCap) {
        pfnGetCudaCap(dev, &devInfo.cudaCapMajor, &devInfo.cudaCapMinor);
        devInfo.cudaCoreCount = getCudaCoreCount(devInfo.cudaCapMajor, devInfo.cudaCapMinor);
      }
      
      if (pfnGetEnforcedPowerLimit) {
        unsigned int limit = 0;
        if (pfnGetEnforcedPowerLimit(dev, &limit) == NVML_SUCCESS)
          devInfo.tdpWatts = limit / 1000;
      }
      
      info.devices.push_back(devInfo);
    }
  }

  // Get global info
  if (pfnGetDriverVer) {
    char ver[80] = {};
    if (pfnGetDriverVer(ver, sizeof(ver)) == NVML_SUCCESS)
      info.driverVersion = ver;
  }

  nvmlLoaded = true;
  info.available = true;
  return true;
}

NvmlMonitor::NvmlMonitor() : hNvml(nullptr), nvmlLoaded(false) {
  processThread = std::thread(&NvmlMonitor::processWorkerLoop, this);
}

NvmlMonitor::~NvmlMonitor() {
  stopProcessThread = true;
  if (processThread.joinable()) processThread.join();
  if (pfnShutdown) pfnShutdown();
  if (hNvml) FreeLibrary(hNvml);
}

int NvmlMonitor::getCudaCoreCount(int major, int minor) const {
  switch (major) {
  case 2: return (minor == 1) ? 48 : 32;
  case 3: return 192;
  case 5: return 128;
  case 6: return (minor == 0) ? 64 : 128;
  case 7: return 64;
  case 8: return (minor == 0) ? 64 : 128;
  case 9: return 128;
  case 10: return 128;
  default: return 128;
  }
}

NvmlInfo NvmlMonitor::getInfo() const {
  std::lock_guard<std::mutex> lock(infoMutex);
  return info;
}

void NvmlMonitor::processWorkerLoop() {
  // Wait for init to complete
  while (!nvmlLoaded && !stopProcessThread) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  while (!stopProcessThread) {
    if (nvmlLoaded && !devices.empty()) {
      // Create a local copy to update without locking
      NvmlInfo localInfo;
      {
        std::lock_guard<std::mutex> lock(infoMutex);
        localInfo = info;
      }

      for (size_t i = 0; i < devices.size(); i++) {
        nvmlDevice_t dev = devices[i];
        if (i >= localInfo.devices.size()) continue;
        NvmlDeviceInfo &dInfo = localInfo.devices[i];

        // 1. Hardware Metrics (Fast)
        if (pfnGetTemp) pfnGetTemp(dev, NVML_TEMPERATURE_GPU, &dInfo.temperatureC);
        if (pfnGetPower && pfnGetPower(dev, &dInfo.powerMilliwatts) == NVML_SUCCESS)
          dInfo.powerWatts = dInfo.powerMilliwatts / 1000.0f;
        
        nvmlUtilization_t util = {};
        if (pfnGetUtil && pfnGetUtil(dev, &util) == NVML_SUCCESS) {
          dInfo.gpuUtil = util.gpu;
          dInfo.memUtil = util.memory;
        }

        nvmlMemory_t mem = {};
        if (pfnGetMemInfo && pfnGetMemInfo(dev, &mem) == NVML_SUCCESS) {
          dInfo.vramTotal = mem.total;
          dInfo.vramUsed = mem.used;
          dInfo.vramFree = mem.free;
        }

        if (pfnGetClock) {
          pfnGetClock(dev, NVML_CLOCK_GRAPHICS, &dInfo.graphicsClock);
          pfnGetClock(dev, NVML_CLOCK_SM, &dInfo.smClock);
          pfnGetClock(dev, NVML_CLOCK_MEM, &dInfo.memClock);
        }
        if (pfnGetMaxClock) pfnGetMaxClock(dev, NVML_CLOCK_GRAPHICS, &dInfo.maxGraphicsClock);
        if (pfnGetFan) pfnGetFan(dev, &dInfo.fanSpeedPercent);
        if (pfnGetCurrPcieLinkGen) pfnGetCurrPcieLinkGen(dev, &dInfo.pcieGeneration);
        if (pfnGetCurrPcieLinkWidth) pfnGetCurrPcieLinkWidth(dev, &dInfo.pcieWidth);
        if (pfnGetPcie) {
          pfnGetPcie(dev, NVML_PCIE_UTIL_RX_BYTES, &dInfo.pcieRxKBs);
          pfnGetPcie(dev, NVML_PCIE_UTIL_TX_BYTES, &dInfo.pcieTxKBs);
        }
        
        unsigned int sap = 0;
        if (pfnGetEncoder) pfnGetEncoder(dev, &dInfo.encoderUtil, &sap);
        if (pfnGetDecoder) pfnGetDecoder(dev, &dInfo.decoderUtil, &sap);

        // 2. Process Enumeration (Slow - only if pfnGetProcesses exists)
        if (pfnGetProcesses) {
          dInfo.gpuProcesses.clear();
          unsigned int procCount = 32;
          nvmlProcessInfo_t procs[32] = {};
          if (pfnGetProcesses(dev, &procCount, procs) == NVML_SUCCESS && procCount > 0) {
            for (unsigned int p = 0; p < procCount; p++) {
              GpuProcessInfo gp;
              gp.pid = procs[p].pid;
              gp.vramUsageBytes = procs[p].usedGpuMemory;
              gp.name = resolveProcessNameWithCache(gp.pid);
              dInfo.gpuProcesses.push_back(gp);
            }
          }
        }
      }

      // 3. Thread-safe swap (Fastest possible lock)
      {
        std::lock_guard<std::mutex> lock(infoMutex);
        info = std::move(localInfo);
      }
    }
    
    // Background polling interval (500ms for smoothness)
    for (int i = 0; i < 5 && !stopProcessThread; i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

void NvmlMonitor::update() {
  // Main thread update is now a no-op.
  // getInfo() provides the latest background-pushed data.
}
