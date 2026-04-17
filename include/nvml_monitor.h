#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <windows.h>
#include <thread>
#include <mutex>
#include <atomic>

// ============================================================================
// NVML Type Definitions
// ============================================================================

typedef void *nvmlDevice_t;

typedef enum {
  NVML_SUCCESS = 0,
  NVML_ERROR_UNINITIALIZED = 1,
  NVML_ERROR_INVALID_ARGUMENT = 2,
  NVML_ERROR_NOT_SUPPORTED = 3,
  NVML_ERROR_NO_PERMISSION = 4,
  NVML_ERROR_NOT_FOUND = 6,
  NVML_ERROR_UNKNOWN = 999
} nvmlReturn_t;

typedef enum { NVML_TEMPERATURE_GPU = 0 } nvmlTemperatureSensors_t;

typedef enum {
  NVML_CLOCK_GRAPHICS = 0,
  NVML_CLOCK_SM = 1,
  NVML_CLOCK_MEM = 2
} nvmlClockType_t;

typedef enum {
  NVML_PCIE_UTIL_TX_BYTES = 0,
  NVML_PCIE_UTIL_RX_BYTES = 1
} nvmlPcieUtilCounter_t;

typedef struct {
  unsigned int gpu;
  unsigned int memory;
} nvmlUtilization_t;

typedef struct {
  unsigned int pid;
  unsigned long long usedGpuMemory;
} nvmlProcessInfo_t;

typedef nvmlReturn_t (*PFN_nvmlInit_v2)();
typedef nvmlReturn_t (*PFN_nvmlShutdown)();
typedef nvmlReturn_t (*PFN_nvmlDeviceGetCount_v2)(unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetHandleByIndex_v2)(unsigned int, nvmlDevice_t *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetTemperature)(nvmlDevice_t, nvmlTemperatureSensors_t, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetPowerUsage)(nvmlDevice_t, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetUtilizationRates)(nvmlDevice_t, nvmlUtilization_t *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetClockInfo)(nvmlDevice_t, nvmlClockType_t, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetFanSpeed)(nvmlDevice_t, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetPcieThroughput)(nvmlDevice_t, nvmlPcieUtilCounter_t, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetEncoderUtilization)(nvmlDevice_t, unsigned int *, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetDecoderUtilization)(nvmlDevice_t, unsigned int *, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetCudaComputeCapability)(nvmlDevice_t, int *, int *);
typedef nvmlReturn_t (*PFN_nvmlSystemGetDriverVersion)(char *, unsigned int);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetComputeRunningProcesses_v3)(nvmlDevice_t, unsigned int *, nvmlProcessInfo_t *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetMaxClockInfo)(nvmlDevice_t, nvmlClockType_t, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetName)(nvmlDevice_t, char *, unsigned int);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetMaxPcieLinkGeneration)(nvmlDevice_t, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetCurrPcieLinkGeneration)(nvmlDevice_t, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetCurrPcieLinkWidth)(nvmlDevice_t, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetEnforcedPowerLimit)(nvmlDevice_t, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetMemoryInfo)(nvmlDevice_t, void *);

struct nvmlMemory_t {
  unsigned long long total;
  unsigned long long free;
  unsigned long long used;
};

struct GpuProcessInfo {
  unsigned int pid = 0;
  uint64_t vramUsageBytes = 0;
  std::string name;
};

struct NvmlDeviceInfo {
  std::string name;
  unsigned int temperatureC = 0;
  unsigned int powerMilliwatts = 0;
  float powerWatts = 0.0f;
  unsigned int tdpWatts = 0;
  unsigned int gpuUtil = 0;
  unsigned int memUtil = 0;
  unsigned int graphicsClock = 0;
  unsigned int maxGraphicsClock = 0;
  unsigned int smClock = 0;
  unsigned int memClock = 0;
  unsigned int fanSpeedPercent = 0;
  unsigned int pcieRxKBs = 0;
  unsigned int pcieTxKBs = 0;
  unsigned int pcieGeneration = 0;
  unsigned int pcieWidth = 0;
  unsigned int encoderUtil = 0;
  unsigned int decoderUtil = 0;
  int cudaCapMajor = 0;
  int cudaCapMinor = 0;
  int cudaCoreCount = 0;
  uint64_t vramTotal = 0;
  uint64_t vramUsed = 0;
  uint64_t vramFree = 0;
  std::vector<GpuProcessInfo> gpuProcesses;
};

struct NvmlInfo {
  bool available = false;
  std::string driverVersion;
  std::vector<NvmlDeviceInfo> devices;
};

class NvmlMonitor {
public:
  NvmlMonitor();
  ~NvmlMonitor();

  bool init();
  void update();
  NvmlInfo getInfo() const;
  bool isAvailable() const { return nvmlLoaded; }

private:
  void processWorkerLoop();
  bool loadLibrary();
  void resolveSymbols();
  int getCudaCoreCount(int major, int minor) const;

  HMODULE hNvml = nullptr;
  std::vector<nvmlDevice_t> devices;
  bool nvmlLoaded = false;
  
  NvmlInfo info;
  mutable std::mutex infoMutex;
  
  std::thread processThread;
  std::atomic<bool> stopProcessThread{false};

  struct CachedProcessName {
    std::string name;
    std::chrono::steady_clock::time_point lastUpdated;
  };
  std::unordered_map<unsigned int, CachedProcessName> processNameCache;
  std::string resolveProcessNameWithCache(unsigned int pid);

  // Function pointers
  PFN_nvmlInit_v2 pfnInit = nullptr;
  PFN_nvmlShutdown pfnShutdown = nullptr;
  PFN_nvmlDeviceGetCount_v2 pfnGetCount = nullptr;
  PFN_nvmlDeviceGetHandleByIndex_v2 pfnGetHandle = nullptr;
  PFN_nvmlDeviceGetTemperature pfnGetTemp = nullptr;
  PFN_nvmlDeviceGetPowerUsage pfnGetPower = nullptr;
  PFN_nvmlDeviceGetUtilizationRates pfnGetUtil = nullptr;
  PFN_nvmlDeviceGetClockInfo pfnGetClock = nullptr;
  PFN_nvmlDeviceGetMaxClockInfo pfnGetMaxClock = nullptr;
  PFN_nvmlDeviceGetFanSpeed pfnGetFan = nullptr;
  PFN_nvmlDeviceGetPcieThroughput pfnGetPcie = nullptr;
  PFN_nvmlDeviceGetEncoderUtilization pfnGetEncoder = nullptr;
  PFN_nvmlDeviceGetDecoderUtilization pfnGetDecoder = nullptr;
  PFN_nvmlDeviceGetCudaComputeCapability pfnGetCudaCap = nullptr;
  PFN_nvmlSystemGetDriverVersion pfnGetDriverVer = nullptr;
  PFN_nvmlDeviceGetComputeRunningProcesses_v3 pfnGetProcesses = nullptr;
  PFN_nvmlDeviceGetName pfnGetName = nullptr;
  PFN_nvmlDeviceGetMemoryInfo pfnGetMemInfo = nullptr;
  PFN_nvmlDeviceGetEnforcedPowerLimit pfnGetEnforcedPowerLimit = nullptr;
  PFN_nvmlDeviceGetMaxPcieLinkGeneration pfnGetMaxPcieLinkGen = nullptr;
  PFN_nvmlDeviceGetCurrPcieLinkGeneration pfnGetCurrPcieLinkGen = nullptr;
  PFN_nvmlDeviceGetCurrPcieLinkWidth pfnGetCurrPcieLinkWidth = nullptr;
};
