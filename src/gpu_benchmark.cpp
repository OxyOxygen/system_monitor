#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "gpu_benchmark.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// ============================================================================
// Cores per SM by Compute Capability
// ============================================================================

int GpuBenchmark::getCoresPerSm(int major, int minor) const {
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
    return 64; // Volta / Turing
  case 8:
    return (minor == 0) ? 64 : 128; // Ampere (A100=64, RTX30xx=128)
  case 9:
    return 128; // Ada Lovelace
  case 10:
    return 128; // Blackwell
  default:
    return 128;
  }
}

// ============================================================================
// Estimate SM Count from GPU Name (common cards)
// ============================================================================

int GpuBenchmark::estimateSmCount(const NvmlDeviceInfo &devInfo) const {
  // Without direct NVML SM count API, we use a lookup for popular GPUs.
  // This gives us total_cores = smCount * coresPerSM for TFLOPS calculation.
  struct GpuSm {
    const char *pattern;
    int smCount;
  };

  static const GpuSm gpuSmTable[] = {
      // Ada Lovelace (RTX 40 series)
      {"4090", 128},
      {"4080 SUPER", 80},
      {"4080", 76},
      {"4070 Ti SUPER", 66},
      {"4070 Ti", 60},
      {"4070 SUPER", 56},
      {"4070", 46},
      {"4060 Ti", 34},
      {"4060", 24},
      // Ampere (RTX 30 series)
      {"3090 Ti", 84},
      {"3090", 82},
      {"3080 Ti", 80},
      {"3080", 68},
      {"3070 Ti", 48},
      {"3070", 46},
      {"3060 Ti", 38},
      {"3060", 28},
      // Turing (RTX 20 series)
      {"2080 Ti", 68},
      {"2080 SUPER", 48},
      {"2080", 46},
      {"2070 SUPER", 40},
      {"2070", 36},
      {"2060 SUPER", 34},
      {"2060", 30},
      // Pascal (GTX 10 series)
      {"1080 Ti", 28},
      {"1080", 20},
      {"1070 Ti", 19},
      {"1070", 15},
      {"1060", 10},
      {"1050 Ti", 6},
      {"1050", 5},
      // Laptop variants (common)
      {"4090 Laptop", 76},
      {"4080 Laptop", 58},
      {"4070 Laptop", 36},
      {"4060 Laptop", 24},
      {"3080 Laptop", 48},
      {"3070 Laptop", 40},
      {"3060 Laptop", 30},
      // NVIDIA A-series / Datacenter
      {"A100", 108},
      {"A6000", 84},
      {"A5000", 64},
      {"A4000", 48},
      {"H100", 132},
      {"L40", 142},
  };

  // Try to match GPU name
  // We need to get GPU name from somewhere — use DXGI or NVML
  // For now, return a reasonable default based on VRAM
  // Actually, let's match against known patterns if we can

  // Default estimation: use VRAM as a rough proxy
  // Typical ratios: entry=24 SM, mid=46 SM, high=82 SM, flagship=128 SM
  return 46; // Default mid-range — will be refined when GPU name is available
}

// ============================================================================
// VRAM Bandwidth Measurement via D3D11
// ============================================================================

float GpuBenchmark::measureVramBandwidth() const {
  // Create D3D11 device
  ID3D11Device *pDevice = nullptr;
  ID3D11DeviceContext *pContext = nullptr;
  D3D_FEATURE_LEVEL featureLevel;

  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                 nullptr, 0, D3D11_SDK_VERSION, &pDevice,
                                 &featureLevel, &pContext);
  if (FAILED(hr))
    return 0.0f;

  // Create two large buffers (128MB each)
  const size_t bufferSize = 128 * 1024 * 1024; // 128 MB

  D3D11_BUFFER_DESC bufDesc = {};
  bufDesc.ByteWidth = static_cast<UINT>(bufferSize);
  bufDesc.Usage = D3D11_USAGE_DEFAULT;
  bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

  ID3D11Buffer *pBufSrc = nullptr;
  ID3D11Buffer *pBufDst = nullptr;

  hr = pDevice->CreateBuffer(&bufDesc, nullptr, &pBufSrc);
  if (FAILED(hr)) {
    pContext->Release();
    pDevice->Release();
    return 0.0f;
  }

  hr = pDevice->CreateBuffer(&bufDesc, nullptr, &pBufDst);
  if (FAILED(hr)) {
    pBufSrc->Release();
    pContext->Release();
    pDevice->Release();
    return 0.0f;
  }

  // Warm up
  for (int i = 0; i < 3; i++) {
    pContext->CopyResource(pBufDst, pBufSrc);
  }
  pContext->Flush();

  // Create query for GPU-side timing
  D3D11_QUERY_DESC queryDesc = {};
  queryDesc.Query = D3D11_QUERY_EVENT;
  ID3D11Query *pQuery = nullptr;
  pDevice->CreateQuery(&queryDesc, &pQuery);

  // Measure
  const int iterations = 20;
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; i++) {
    pContext->CopyResource(pBufDst, pBufSrc);
  }

  // Signal end
  if (pQuery) {
    pContext->End(pQuery);
    // Wait for GPU to finish
    BOOL queryData = FALSE;
    while (pContext->GetData(pQuery, &queryData, sizeof(queryData), 0) !=
           S_OK) {
      // Busy wait
    }
    pQuery->Release();
  } else {
    pContext->Flush();
  }

  auto end = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();

  // Calculate bandwidth (read + write = 2x buffer size per copy)
  double totalBytes = 2.0 * bufferSize * iterations;
  float bandwidthGBs = static_cast<float>(totalBytes / elapsed / 1e9);

  // Cleanup
  pBufSrc->Release();
  pBufDst->Release();
  pContext->Release();
  pDevice->Release();

  return bandwidthGBs;
}

// ============================================================================
// Score Calculation
// ============================================================================

int GpuBenchmark::calculateScore(const BenchmarkResult &result) const {
  // Score based on FP32 TFLOPS and bandwidth
  // Thresholds:
  // 0-20:   Basic (< 5 TFLOPS, no AI)
  // 20-40:  Entry AI (5-15 TFLOPS, small models)
  // 40-60:  Mid-range (15-30 TFLOPS, 7B models)
  // 60-80:  High-end (30-60 TFLOPS, 13-30B models)
  // 80-100: Flagship (60+ TFLOPS, 70B+ models)

  float tflops = result.fp32Tflops;
  int score = 0;

  if (tflops >= 80.0f)
    score = 95;
  else if (tflops >= 60.0f)
    score = 80 + static_cast<int>((tflops - 60.0f) / 20.0f * 15.0f);
  else if (tflops >= 30.0f)
    score = 60 + static_cast<int>((tflops - 30.0f) / 30.0f * 20.0f);
  else if (tflops >= 15.0f)
    score = 40 + static_cast<int>((tflops - 15.0f) / 15.0f * 20.0f);
  else if (tflops >= 5.0f)
    score = 20 + static_cast<int>((tflops - 5.0f) / 10.0f * 20.0f);
  else
    score = static_cast<int>(tflops / 5.0f * 20.0f);

  // Bonus for bandwidth
  if (result.vramBandwidthGBs > 500.0f)
    score = std::min(100, score + 5);
  else if (result.vramBandwidthGBs > 300.0f)
    score = std::min(100, score + 3);

  // Bonus for tensor cores
  if (result.hasTensorCores)
    score = std::min(100, score + 5);

  return std::max(0, std::min(100, score));
}

std::string GpuBenchmark::generateScoreLabel(int score) const {
  if (score >= 90)
    return "EXCEPTIONAL — Flagship AI Platform";
  if (score >= 75)
    return "EXCELLENT — High-End AI Workloads";
  if (score >= 60)
    return "VERY GOOD — Mid-Large Model Training";
  if (score >= 45)
    return "GOOD — 7B-13B Model Inference";
  if (score >= 30)
    return "FAIR — Small Model Inference";
  if (score >= 15)
    return "BASIC — Limited AI Capability";
  return "MINIMAL — Not Recommended for AI";
}

std::string GpuBenchmark::getCurrentTimestamp() const {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  struct tm timeinfo;
  localtime_s(&timeinfo, &t);
  std::ostringstream oss;
  oss << std::put_time(&timeinfo, "%H:%M:%S");
  return oss.str();
}

// ============================================================================
// Run Benchmark
// ============================================================================

BenchmarkResult GpuBenchmark::runBenchmark(const NvmlDeviceInfo &devInfo) {
  running = true;
  BenchmarkResult result = {};

  // Get SM count and cores per SM
  int smCount = estimateSmCount(devInfo);
  int coresPerSm = getCoresPerSm(devInfo.cudaCapMajor, devInfo.cudaCapMinor);
  int totalCores = devInfo.cudaCoreCount > 0 ? devInfo.cudaCoreCount : (smCount * coresPerSm);

  result.smCount = smCount;
  result.cudaCores = totalCores;
  result.clockMHz = devInfo.graphicsClock > 0 ? devInfo.graphicsClock : (devInfo.maxGraphicsClock > 0 ? devInfo.maxGraphicsClock : 1500);

  // Tensor cores: available from Volta (7.0) onwards
  result.hasTensorCores = (devInfo.cudaCapMajor >= 7);

  // Theoretical TFLOPS
  // FP32: cores * clock * 2 (FMA) / 1e6
  result.fp32Tflops = (totalCores * result.clockMHz * 2.0f) / 1e6f;

  // FP16: double FP32 on most modern GPUs (Tensor Cores do even better)
  result.fp16Tflops = result.fp32Tflops * 2.0f;
  if (result.hasTensorCores)
    result.fp16Tflops *= 2.0f; // Tensor core boost

  // INT8: 4x FP32 on tensor-core GPUs
  result.int8Tops = result.hasTensorCores ? result.fp32Tflops * 4.0f
                                          : result.fp32Tflops * 2.0f;

  // VRAM Bandwidth — measured via D3D11
  result.vramBandwidthGBs = measureVramBandwidth();
  result.bandwidthMeasured = (result.vramBandwidthGBs > 0.0f);

  // Score
  result.score = calculateScore(result);
  result.scoreLabel = generateScoreLabel(result.score);
  result.timestamp = getCurrentTimestamp();
  result.valid = true;

  lastResult = result;

  // Keep last 5 results
  history.push_back(result);
  if (history.size() > 5)
    history.erase(history.begin());

  running = false;
  return result;
}
