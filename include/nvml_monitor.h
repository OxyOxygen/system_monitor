#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>


// ============================================================================
// NVML Type Definitions (matches NVIDIA's nvml.h without requiring the header)
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
  unsigned int gpu;    // GPU utilization %
  unsigned int memory; // Memory utilization %
} nvmlUtilization_t;

typedef struct {
  unsigned int pid;
  unsigned long long usedGpuMemory;
} nvmlProcessInfo_t;

// ============================================================================
// NVML Function Pointer Types
// ============================================================================

typedef nvmlReturn_t (*PFN_nvmlInit_v2)();
typedef nvmlReturn_t (*PFN_nvmlShutdown)();
typedef nvmlReturn_t (*PFN_nvmlDeviceGetCount_v2)(unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetHandleByIndex_v2)(unsigned int,
                                                          nvmlDevice_t *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetTemperature)(nvmlDevice_t,
                                                     nvmlTemperatureSensors_t,
                                                     unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetPowerUsage)(nvmlDevice_t,
                                                    unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetUtilizationRates)(nvmlDevice_t,
                                                          nvmlUtilization_t *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetClockInfo)(nvmlDevice_t,
                                                   nvmlClockType_t,
                                                   unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetFanSpeed)(nvmlDevice_t, unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetPcieThroughput)(nvmlDevice_t,
                                                        nvmlPcieUtilCounter_t,
                                                        unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetEncoderUtilization)(nvmlDevice_t,
                                                            unsigned int *,
                                                            unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetDecoderUtilization)(nvmlDevice_t,
                                                            unsigned int *,
                                                            unsigned int *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetCudaComputeCapability)(nvmlDevice_t,
                                                               int *, int *);
typedef nvmlReturn_t (*PFN_nvmlSystemGetDriverVersion)(char *, unsigned int);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetComputeRunningProcesses_v3)(
    nvmlDevice_t, unsigned int *, nvmlProcessInfo_t *);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetName)(nvmlDevice_t, char *,
                                              unsigned int);
typedef nvmlReturn_t (*PFN_nvmlDeviceGetMemoryInfo)(nvmlDevice_t, void *);

// ============================================================================
// GPU Process Info (for display)
// ============================================================================

struct GpuProcessInfo {
  unsigned int pid = 0;
  uint64_t vramUsageBytes = 0;
  std::string name;
};

// ============================================================================
// NVML Info Struct
// ============================================================================

struct NvmlInfo {
  bool available = false;

  // Temperature & Power
  unsigned int temperatureC = 0;
  unsigned int powerMilliwatts = 0;
  float powerWatts = 0.0f;

  // Utilization
  unsigned int gpuUtil = 0; // GPU compute %
  unsigned int memUtil = 0; // Memory interface %

  // Clocks (MHz)
  unsigned int graphicsClock = 0;
  unsigned int smClock = 0;
  unsigned int memClock = 0;

  // Fan
  unsigned int fanSpeedPercent = 0;

  // PCIe (KB/s)
  unsigned int pcieRxKBs = 0;
  unsigned int pcieTxKBs = 0;

  // Encoder / Decoder
  unsigned int encoderUtil = 0;
  unsigned int decoderUtil = 0;

  // CUDA
  int cudaCapMajor = 0;
  int cudaCapMinor = 0;
  int cudaCoreCount = 0;

  // Driver
  std::string driverVersion;

  // Active GPU Processes
  std::vector<GpuProcessInfo> gpuProcesses;
};

// ============================================================================
// NvmlMonitor Class
// ============================================================================

class NvmlMonitor {
public:
  NvmlMonitor() = default;
  ~NvmlMonitor();

  bool init();
  void update();
  const NvmlInfo &getInfo() const { return info; }
  bool isAvailable() const { return nvmlLoaded; }

private:
  bool loadLibrary();
  void resolveSymbols();
  int getCudaCoreCount(int major, int minor) const;

  HMODULE hNvml = nullptr;
  nvmlDevice_t device = nullptr;
  bool nvmlLoaded = false;
  NvmlInfo info;

  // Function pointers
  PFN_nvmlInit_v2 pfnInit = nullptr;
  PFN_nvmlShutdown pfnShutdown = nullptr;
  PFN_nvmlDeviceGetCount_v2 pfnGetCount = nullptr;
  PFN_nvmlDeviceGetHandleByIndex_v2 pfnGetHandle = nullptr;
  PFN_nvmlDeviceGetTemperature pfnGetTemp = nullptr;
  PFN_nvmlDeviceGetPowerUsage pfnGetPower = nullptr;
  PFN_nvmlDeviceGetUtilizationRates pfnGetUtil = nullptr;
  PFN_nvmlDeviceGetClockInfo pfnGetClock = nullptr;
  PFN_nvmlDeviceGetFanSpeed pfnGetFan = nullptr;
  PFN_nvmlDeviceGetPcieThroughput pfnGetPcie = nullptr;
  PFN_nvmlDeviceGetEncoderUtilization pfnGetEncoder = nullptr;
  PFN_nvmlDeviceGetDecoderUtilization pfnGetDecoder = nullptr;
  PFN_nvmlDeviceGetCudaComputeCapability pfnGetCudaCap = nullptr;
  PFN_nvmlSystemGetDriverVersion pfnGetDriverVer = nullptr;
  PFN_nvmlDeviceGetComputeRunningProcesses_v3 pfnGetProcesses = nullptr;
  PFN_nvmlDeviceGetName pfnGetName = nullptr;
  PFN_nvmlDeviceGetMemoryInfo pfnGetMemInfo = nullptr;
};
