#include "nvml_monitor.h"
#include <algorithm>
#include <tlhelp32.h>

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

  // Get first device
  unsigned int deviceCount = 0;
  if (pfnGetCount)
    pfnGetCount(&deviceCount);

  if (deviceCount == 0) {
    pfnShutdown();
    FreeLibrary(hNvml);
    hNvml = nullptr;
    return false;
  }

  if (pfnGetHandle(0, &device) != NVML_SUCCESS) {
    pfnShutdown();
    FreeLibrary(hNvml);
    hNvml = nullptr;
    return false;
  }

  // Get static info (driver, CUDA capability)
  if (pfnGetDriverVer) {
    char ver[80] = {};
    if (pfnGetDriverVer(ver, sizeof(ver)) == NVML_SUCCESS)
      info.driverVersion = ver;
  }

  if (pfnGetCudaCap) {
    pfnGetCudaCap(device, &info.cudaCapMajor, &info.cudaCapMinor);
    info.cudaCoreCount = getCudaCoreCount(info.cudaCapMajor, info.cudaCapMinor);
  }

  nvmlLoaded = true;
  info.available = true;
  return true;
}

NvmlMonitor::~NvmlMonitor() {
  if (pfnShutdown)
    pfnShutdown();
  if (hNvml)
    FreeLibrary(hNvml);
}

// ============================================================================
// CUDA Core Count Lookup
// ============================================================================

int NvmlMonitor::getCudaCoreCount(int major, int minor) const {
  // SM count is not directly available from NVML, so we estimate CUDA cores
  // based on compute capability. This is the cores-per-SM for each arch.
  // Actual total = SM_count * cores_per_SM, but NVML doesn't expose SM count.
  // We'll return cores_per_SM as a reference, and calculate total TFLOPS
  // in benchmark using clock speeds.

  // Cores per SM by compute capability
  switch (major) {
  case 2:
    return (minor == 1) ? 48 : 32; // Fermi
  case 3:
    return 192; // Kepler
  case 5:
    return 128; // Maxwell
  case 6:
    return (minor == 0) ? 64 : 128; // Pascal
  case 7:
    return 64; // Volta/Turing
  case 8:
    return (minor == 0) ? 64 : 128; // Ampere
  case 9:
    return 128; // Ada Lovelace
  case 10:
    return 128; // Blackwell
  default:
    return 128; // Future architectures, guess
  }
}

// ============================================================================
// Update — Query All Metrics
// ============================================================================

void NvmlMonitor::update() {
  if (!nvmlLoaded || !device)
    return;

  // Temperature
  if (pfnGetTemp)
    pfnGetTemp(device, NVML_TEMPERATURE_GPU, &info.temperatureC);

  // Power (milliwatts)
  if (pfnGetPower) {
    pfnGetPower(device, &info.powerMilliwatts);
    info.powerWatts = info.powerMilliwatts / 1000.0f;
  }

  // Utilization
  if (pfnGetUtil) {
    nvmlUtilization_t util = {};
    if (pfnGetUtil(device, &util) == NVML_SUCCESS) {
      info.gpuUtil = util.gpu;
      info.memUtil = util.memory;
    }
  }

  // Clocks
  if (pfnGetClock) {
    pfnGetClock(device, NVML_CLOCK_GRAPHICS, &info.graphicsClock);
    pfnGetClock(device, NVML_CLOCK_SM, &info.smClock);
    pfnGetClock(device, NVML_CLOCK_MEM, &info.memClock);
  }

  if (pfnGetMaxClock) {
    pfnGetMaxClock(device, NVML_CLOCK_GRAPHICS, &info.maxGraphicsClock);
  }

  // Fan
  if (pfnGetFan)
    pfnGetFan(device, &info.fanSpeedPercent);

  // PCIe Throughput
  if (pfnGetPcie) {
    pfnGetPcie(device, NVML_PCIE_UTIL_RX_BYTES, &info.pcieRxKBs);
    pfnGetPcie(device, NVML_PCIE_UTIL_TX_BYTES, &info.pcieTxKBs);
  }

  // Encoder / Decoder
  unsigned int samplingPeriod = 0;
  if (pfnGetEncoder)
    pfnGetEncoder(device, &info.encoderUtil, &samplingPeriod);
  if (pfnGetDecoder)
    pfnGetDecoder(device, &info.decoderUtil, &samplingPeriod);

  // GPU Compute Processes (Commented out frequent snapshots to save
  // performance)
  /*
  if (pfnGetProcesses) {
    info.gpuProcesses.clear();
    unsigned int procCount = 32;
    nvmlProcessInfo_t procs[32] = {};

    nvmlReturn_t ret = pfnGetProcesses(device, &procCount, procs);
    if (ret == NVML_SUCCESS && procCount > 0) {
      // Build a PID→name map from system snapshot
      HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
      std::vector<std::pair<DWORD, std::string>> pidNames;

      if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(snapshot, &pe)) {
          do {
            char name[MAX_PATH] = {};
            WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, name,
                                 sizeof(name), nullptr, nullptr);
            pidNames.push_back({pe.th32ProcessID, name});
          } while (Process32NextW(snapshot, &pe));
        }
        CloseHandle(snapshot);
      }

      for (unsigned int i = 0; i < procCount; i++) {
        GpuProcessInfo gp;
        gp.pid = procs[i].pid;
        gp.vramUsageBytes = procs[i].usedGpuMemory;

        // Lookup name
        for (const auto &pn : pidNames) {
          if (pn.first == gp.pid) {
            gp.name = pn.second;
            break;
          }
        }
        if (gp.name.empty())
          gp.name = "Unknown";

        info.gpuProcesses.push_back(gp);
      }
    }
  }
  */
}
