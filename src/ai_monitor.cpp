#include "ai_monitor.h"
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <psapi.h>
#include <sstream>
#include <tlhelp32.h>

// ─── AI Process Detection Patterns ──────────────────────────────────────────
static const char *AI_PROCESS_PATTERNS[] = {
    "python",
    "python3",
    "pythonw",
    "ollama",
    "ollama_llama_server",
    "tritonserver",
    "vllm",
    "llama",
    "llama-server",
    "llama-cli",
    "stable-diffusion",
    "comfyui",
    "automatic1111",
    "kohya",
    "text-generation",
    "tgi",
    "jupyter",
    "jupyter-lab",
    "jupyter-notebook",
    "ipykernel",
    "tensorboard",
    "mlflow",
    "ray",
    "torchserve",
    "tf_serving",
    "onnxruntime",
    "whisper",
    "bark",
    "localai",
    "lmstudio",
    "koboldcpp",
    "gpt4all",
    "jan",
    "chatd",
    "llamafile",
    "tabbyml",
    "continue",
    "copilot",
    "cursor",
    nullptr // sentinel
};

// ─── Helper: Check if process name matches AI patterns ──────────────────────
bool AiMonitor::isAiProcess(const std::string &name) {
  // Convert to lowercase for comparison
  std::string lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  // Remove .exe extension if present
  std::string baseName = lower;
  size_t extPos = baseName.rfind(".exe");
  if (extPos != std::string::npos && extPos == baseName.length() - 4) {
    baseName = baseName.substr(0, extPos);
  }

  for (int i = 0; AI_PROCESS_PATTERNS[i] != nullptr; i++) {
    if (baseName.find(AI_PROCESS_PATTERNS[i]) != std::string::npos) {
      return true;
    }
  }
  return false;
}

// ─── Constructor ─────────────────────────────────────────────────────────────
AiMonitor::AiMonitor() {
  info = {};
  info.totalAiProcesses = 0;
  info.gpuAvailable = false;
  info.cudaDetected = false;
  info.pythonDetected = false;
  info.workloadStatus = "No AI Workload";
  info.workloadStatusDesc = "No AI processes detected";

  // One-time environment detection
  detectEnvironment();
}

// ─── Detect AI Processes via Windows Toolhelp ────────────────────────────────
void AiMonitor::detectAiProcesses() {
  info.processes.clear();

  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    info.totalAiProcesses = 0;
    return;
  }

  PROCESSENTRY32W pe;
  pe.dwSize = sizeof(PROCESSENTRY32W);

  if (Process32FirstW(snapshot, &pe)) {
    do {
      char procName[MAX_PATH] = {};
      WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, procName,
                          sizeof(procName), nullptr, nullptr);

      if (isAiProcess(procName)) {
        AiProcessInfo aiProc;
        aiProc.pid = pe.th32ProcessID;
        aiProc.name = procName;
        aiProc.memoryUsed = 0;

        // Get memory usage
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                   FALSE, pe.th32ProcessID);
        if (hProc) {
          PROCESS_MEMORY_COUNTERS pmc;
          if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc))) {
            aiProc.memoryUsed = pmc.WorkingSetSize;
          }
          CloseHandle(hProc);
        }

        info.processes.push_back(aiProc);
      }
    } while (Process32NextW(snapshot, &pe));
  }

  CloseHandle(snapshot);
  info.totalAiProcesses = static_cast<int>(info.processes.size());

  // Sort by memory usage (descending)
  std::sort(info.processes.begin(), info.processes.end(),
            [](const AiProcessInfo &a, const AiProcessInfo &b) {
              return a.memoryUsed > b.memoryUsed;
            });
}

// ─── Update GPU Metrics ─────────────────────────────────────────────────────
void AiMonitor::updateGpuMetrics(const GpuInfo &gpuInfo) {
  info.gpuName = gpuInfo.gpuName;
  info.gpuUsagePercent = gpuInfo.gpuUsage;
  info.vramUsed = gpuInfo.vramUsed;
  info.vramTotal = gpuInfo.vramTotal;
  info.vramUsagePercent = gpuInfo.vramUsagePercent;
  info.vramFree = (gpuInfo.vramTotal > gpuInfo.vramUsed)
                      ? (gpuInfo.vramTotal - gpuInfo.vramUsed)
                      : 0;
  info.temperatureC = gpuInfo.temperatureC;
  info.powerDrawWatts = gpuInfo.powerDrawWatts;
  info.tdpWatts = gpuInfo.tdpWatts;
  info.gpuAvailable = gpuInfo.available;

  // Estimate AI power: if AI processes are running, attribute GPU power
  if (info.totalAiProcesses > 0 && gpuInfo.available) {
    info.aiPowerEstimateW =
        gpuInfo.powerDrawWatts * 0.85; // ~85% attributed to AI
  } else {
    info.aiPowerEstimateW = 0.0;
  }
}

// ─── Evaluate Model Capabilities ────────────────────────────────────────────
void AiMonitor::evaluateModelCapabilities() {
  info.modelCapabilities.clear();

  double freeVramGB =
      static_cast<double>(info.vramFree) / (1024.0 * 1024.0 * 1024.0);
  double totalVramGB =
      static_cast<double>(info.vramTotal) / (1024.0 * 1024.0 * 1024.0);

  // Model database: name, category, required VRAM (GB) for quantized inference
  struct ModelEntry {
    const char *name;
    const char *category;
    double vramGB;
  };

  static const ModelEntry models[] = {
      // LLMs
      {"Llama 3.2 1B (Q4)", "LLM", 1.5},
      {"Llama 3.2 3B (Q4)", "LLM", 2.5},
      {"Phi-3 Mini 3.8B (Q4)", "LLM", 3.0},
      {"Mistral 7B (Q4)", "LLM", 4.5},
      {"Llama 3.1 8B (Q4)", "LLM", 5.0},
      {"Gemma 2 9B (Q4)", "LLM", 6.0},
      {"GPT-J 6B (FP16)", "LLM", 12.0},
      {"Llama 3.1 13B (Q4)", "LLM", 8.0},
      {"CodeLlama 34B (Q4)", "LLM", 20.0},
      {"Mixtral 8x7B (Q4)", "LLM", 26.0},
      {"Llama 3.1 70B (Q4)", "LLM", 40.0},
      // Vision / Multimodal
      {"LLaVA 7B (Q4)", "Vision", 5.0},
      {"LLaVA 13B (Q4)", "Vision", 8.5},
      {"CogVLM2 (Q4)", "Vision", 10.0},
      // Image Generation
      {"Stable Diffusion 1.5", "Diffusion", 4.0},
      {"SDXL (FP16)", "Diffusion", 6.5},
      {"FLUX.1 Schnell", "Diffusion", 8.0},
      {"FLUX.1 Dev", "Diffusion", 12.0},
      // Audio
      {"Whisper Small", "Audio", 1.0},
      {"Whisper Medium", "Audio", 2.5},
      {"Whisper Large v3", "Audio", 4.0},
      {"Bark TTS", "Audio", 5.0},
  };

  for (const auto &m : models) {
    ModelCapability cap;
    cap.name = m.name;
    cap.category = m.category;
    cap.requiredVramGB = m.vramGB;
    // Can run if total VRAM is enough (with some headroom)
    cap.canRun = (totalVramGB >= m.vramGB);
    info.modelCapabilities.push_back(cap);
  }
}

// ─── Determine Workload Status ──────────────────────────────────────────────
void AiMonitor::determineWorkloadStatus() {
  if (info.totalAiProcesses == 0) {
    info.workloadStatus = "No AI Workload";
    info.workloadStatusDesc = "No AI/ML processes detected on this system";
  } else if (info.gpuAvailable && info.gpuUsagePercent > 10.0) {
    info.workloadStatus = "Active";
    std::ostringstream oss;
    oss << info.totalAiProcesses << " AI process"
        << (info.totalAiProcesses > 1 ? "es" : "") << " running, GPU at "
        << std::fixed << std::setprecision(0) << info.gpuUsagePercent << "%";
    info.workloadStatusDesc = oss.str();
  } else {
    info.workloadStatus = "Idle";
    std::ostringstream oss;
    oss << info.totalAiProcesses << " AI process"
        << (info.totalAiProcesses > 1 ? "es" : "") << " detected, GPU idle";
    info.workloadStatusDesc = oss.str();
  }
}

// ─── Detect Environment (CUDA, Python) ──────────────────────────────────────
void AiMonitor::detectEnvironment() {
  // Check CUDA
  char *cudaPathEnv = nullptr;
  size_t len = 0;
  if (_dupenv_s(&cudaPathEnv, &len, "CUDA_PATH") == 0 && cudaPathEnv) {
    info.cudaDetected = true;
    info.cudaPath = cudaPathEnv;
    free(cudaPathEnv);
  } else {
    info.cudaDetected = false;
    info.cudaPath = "Not detected";
  }

  // Check Python
  char *pathEnv = nullptr;
  if (_dupenv_s(&pathEnv, &len, "PATH") == 0 && pathEnv) {
    std::string pathStr = pathEnv;
    free(pathEnv);
    // Simple check: look for python in PATH
    std::string lowerPath = pathStr;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                   ::tolower);
    if (lowerPath.find("python") != std::string::npos ||
        lowerPath.find("conda") != std::string::npos ||
        lowerPath.find("anaconda") != std::string::npos) {
      info.pythonDetected = true;
      info.pythonVersion = "Detected in PATH";
    } else {
      info.pythonDetected = false;
      info.pythonVersion = "Not detected";
    }
  }
}

// ─── Build Compute Summary ──────────────────────────────────────────────────
void AiMonitor::buildComputeSummary() {
  std::ostringstream oss;

  if (info.gpuAvailable) {
    double totalGB =
        static_cast<double>(info.vramTotal) / (1024.0 * 1024.0 * 1024.0);
    oss << std::fixed << std::setprecision(0) << totalGB << " GB VRAM";
  } else {
    oss << "No GPU";
  }

  if (info.cudaDetected) {
    oss << "  •  CUDA Ready";
  }

  if (info.totalAiProcesses > 0) {
    oss << "  •  " << info.totalAiProcesses << " AI Process"
        << (info.totalAiProcesses > 1 ? "es" : "");
  }

  info.computeSummary = oss.str();
}

// ─── Main Update ─────────────────────────────────────────────────────────────
void AiMonitor::update(const GpuInfo &gpuInfo) {
  detectAiProcesses();
  updateGpuMetrics(gpuInfo);
  evaluateModelCapabilities();
  determineWorkloadStatus();
  buildComputeSummary();
}

// ─── Getter ──────────────────────────────────────────────────────────────────
AiInfo AiMonitor::getAiInfo() const { return info; }
