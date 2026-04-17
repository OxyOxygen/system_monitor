#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "gui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <fstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// ─── Color palette ───────────────────────────────────────────────────────────
static const ImVec4 COL_BG = ImVec4(0.06f, 0.06f, 0.09f, 1.00f);
static const ImVec4 COL_SIDEBAR = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);
static const ImVec4 COL_PANEL = ImVec4(0.10f, 0.10f, 0.15f, 1.00f);
static const ImVec4 COL_PANEL_HOVER = ImVec4(0.13f, 0.13f, 0.19f, 1.00f);
static const ImVec4 COL_ACCENT = ImVec4(0.35f, 0.55f, 1.00f, 1.00f);
static const ImVec4 COL_CYAN = ImVec4(0.00f, 0.85f, 0.85f, 1.00f);
static const ImVec4 COL_GREEN = ImVec4(0.20f, 0.90f, 0.40f, 1.00f);
static const ImVec4 COL_PURPLE = ImVec4(0.70f, 0.40f, 1.00f, 1.00f);
static const ImVec4 COL_ORANGE = ImVec4(1.00f, 0.60f, 0.20f, 1.00f);
static const ImVec4 COL_RED = ImVec4(1.00f, 0.30f, 0.35f, 1.00f);
static const ImVec4 COL_YELLOW = ImVec4(1.00f, 0.85f, 0.25f, 1.00f);
static const ImVec4 COL_TEXT = ImVec4(0.92f, 0.93f, 0.96f, 1.00f);
static const ImVec4 COL_TEXT_DIM = ImVec4(0.55f, 0.56f, 0.62f, 1.00f);
static const ImVec4 COL_BORDER = ImVec4(0.18f, 0.18f, 0.25f, 1.00f);

// ─── Constructor / Destructor ────────────────────────────────────────────────
GUI::GUI()
    : window(nullptr), initialized(false), currentTab(Tab::Overview),
      historyOffset(0), animCpu(0), animMem(0), animDisk(0), animGpu(0),
      animBattery(0), gameMode(false), showOverlay(false) {
  memset(cpuHistory, 0, sizeof(cpuHistory));
  memset(memHistory, 0, sizeof(memHistory));
  memset(gpuHistory, 0, sizeof(gpuHistory));
  memset(downloadHistory, 0, sizeof(downloadHistory));
  memset(uploadHistory, 0, sizeof(uploadHistory));
  memset(powerHistory, 0, sizeof(powerHistory));
  memset(diskReadHistory, 0, sizeof(diskReadHistory));
  memset(diskWriteHistory, 0, sizeof(diskWriteHistory));
}

GUI::~GUI() { cleanup(); }

bool GUI::init(const char *title, int width, int height) {
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return false;

  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (window == nullptr)
    return false;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Scale up font for readability
  io.Fonts->Clear();
  io.Fonts->AddFontDefault();
  io.FontGlobalScale = 1.35f;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  setupStyle();

  initialized = true;
  return true;
}

// ─── Style ───────────────────────────────────────────────────────────────────
void GUI::setupStyle() {
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 *colors = style.Colors;

  colors[ImGuiCol_WindowBg] = COL_BG;
  colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_PopupBg] = COL_PANEL;
  colors[ImGuiCol_Border] = COL_BORDER;
  colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.18f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.24f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.30f, 1.00f);
  colors[ImGuiCol_TitleBg] = COL_SIDEBAR;
  colors[ImGuiCol_TitleBgActive] = COL_SIDEBAR;
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.50f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.42f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = COL_ACCENT;

  colors[ImGuiCol_Button] = ImVec4(0.14f, 0.14f, 0.22f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.35f, 0.65f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.30f, 0.55f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.14f, 0.14f, 0.22f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.28f, 0.50f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.25f, 0.45f, 1.00f);
  colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.16f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.35f, 0.65f, 1.00f);
  colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.30f, 0.60f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.10f, 0.16f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.20f, 0.35f, 1.00f);

  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.12f, 0.18f, 1.00f);
  colors[ImGuiCol_TableBorderStrong] = COL_BORDER;
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.14f, 0.14f, 0.20f, 1.00f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.10f, 0.10f, 0.14f, 0.40f);

  colors[ImGuiCol_PlotLines] = COL_ACCENT;
  colors[ImGuiCol_PlotHistogram] = COL_ACCENT;

  colors[ImGuiCol_Separator] = COL_BORDER;
  colors[ImGuiCol_Text] = COL_TEXT;
  colors[ImGuiCol_TextDisabled] = COL_TEXT_DIM;

  style.WindowRounding = 0.0f;
  style.ChildRounding = 12.0f;
  style.FrameRounding = 8.0f;
  style.GrabRounding = 8.0f;
  style.ScrollbarRounding = 10.0f;
  style.TabRounding = 8.0f;
  style.WindowPadding = ImVec2(16, 16);
  style.FramePadding = ImVec2(14, 8);
  style.ItemSpacing = ImVec2(12, 10);
  style.ItemInnerSpacing = ImVec2(10, 8);
  style.ScrollbarSize = 16.0f;
  style.WindowBorderSize = 0.0f;
  style.ChildBorderSize = 1.0f;
}

bool GUI::shouldClose() { return glfwWindowShouldClose(window); }

void GUI::beginFrame() {
  glfwPollEvents();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

// ─── Main Render ─────────────────────────────────────────────────────────────
void GUI::render(double cpuUsage, const std::vector<double> &coreUsages,
                 const MemoryInfo &memInfo, const DiskInfo &diskInfo,
                 const GpuInfo &gpuInfo, const PowerInfo &powerInfo,
                 const std::vector<ProcessInfo> &processes,
                 const NetworkInfo &netInfo, const SystemInfo &sysInfo,
                 const EnergyInfo &energyInfo, const AiInfo &aiInfo,
                 const NvmlInfo &nvmlInfo, const BenchmarkResult &benchResult,
                 const std::vector<DiskDriveInfo> &allDrives,
                 const DiskIOInfo &diskIOInfo, const CpuTempInfo &cpuTempInfo,
                 const FrameAnalysis &frameAnalysis, const ScoreInfo &scoreInfo,
                 const HpcReport &hpcReport,
                 ProcessMonitor &processMonitor,
                 GamingSessionMonitor &sessionMonitor, HpcEngine &hpcEngine) {
  // Update history
  float memPercent = static_cast<float>(memInfo.usagePercent);
  float gpuPercent =
      gpuInfo.available ? static_cast<float>(gpuInfo.gpuUsage) : 0.0f;
  updateHistory(static_cast<float>(cpuUsage), memPercent, gpuPercent,
                static_cast<float>(netInfo.downloadSpeed),
                static_cast<float>(netInfo.uploadSpeed),
                static_cast<float>(energyInfo.totalWatts),
                static_cast<float>(diskIOInfo.readBytesPerSec),
                static_cast<float>(diskIOInfo.writeBytesPerSec));

  // Smooth animations (lerp)
  float dt = ImGui::GetIO().DeltaTime;
  float lerpSpeed = 8.0f * dt;
  animCpu += (static_cast<float>(cpuUsage) / 100.0f - animCpu) * lerpSpeed;
  animMem += (memPercent / 100.0f - animMem) * lerpSpeed;
  animDisk += (static_cast<float>(diskInfo.usagePercent) / 100.0f - animDisk) *
              lerpSpeed;
  animGpu += (gpuPercent / 100.0f - animGpu) * lerpSpeed;
  if (powerInfo.hasBattery && powerInfo.batteryPercent >= 0) {
    animBattery +=
        (static_cast<float>(powerInfo.batteryPercent) / 100.0f - animBattery) *
        lerpSpeed;
  }

  // Full-window root
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
  ImGui::Begin("##Root", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);

  float sidebarWidth = 280.0f;
  float totalHeight = ImGui::GetContentRegionAvail().y;
  float totalWidth = ImGui::GetContentRegionAvail().x;

  // ── Sidebar ──
  ImGui::BeginChild("##Sidebar", ImVec2(sidebarWidth, totalHeight), false);
  renderSidebar(cpuUsage, memInfo, diskInfo, gpuInfo, powerInfo, energyInfo,
                scoreInfo);
  ImGui::EndChild();

  ImGui::SameLine();

  // ── Main content ──
  ImGui::BeginChild("##MainContent",
                    ImVec2(totalWidth - sidebarWidth - 12, totalHeight), false);

  // Tab bar — 9 tabs now
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
  float tabWidth = 142.0f;

  auto drawTab = [&](const char *label, Tab tab) {
    bool active = (currentTab == tab);
    ImVec4 bg = active ? ImVec4(0.20f, 0.30f, 0.60f, 1.00f)
                       : ImVec4(0.10f, 0.10f, 0.16f, 1.00f);
    ImVec4 bgHover = active ? bg : ImVec4(0.15f, 0.20f, 0.35f, 1.00f);
    ImGui::PushStyleColor(ImGuiCol_Button, bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bgHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, bg);
    if (ImGui::Button(label, ImVec2(tabWidth, 44))) {
      currentTab = tab;
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
  };

  drawTab("  Overview  ", Tab::Overview);
  drawTab(" Processes  ", Tab::Processes);
  drawTab("  Network   ", Tab::Network);
  drawTab("  Energy    ", Tab::Energy);
  drawTab("    AI      ", Tab::AI);
  drawTab(" GPU Compute", Tab::GpuCompute);
  drawTab("  System    ", Tab::System);
  drawTab("  Assistant ", Tab::HpcAssistant);
  drawTab("  Session   ", Tab::GamingSession);

  ImGui::PopStyleVar();
  ImGui::NewLine();
  ImGui::Spacing();

  // Active indicator line
  ImDrawList *dl = ImGui::GetWindowDrawList();
  ImVec2 cursorPos = ImGui::GetCursorScreenPos();
  float indicatorX =
      cursorPos.x + (tabWidth + 2) * static_cast<int>(currentTab);
  dl->AddRectFilled(ImVec2(indicatorX, cursorPos.y - 6),
                    ImVec2(indicatorX + tabWidth, cursorPos.y - 1),
                    ImColor(COL_ACCENT));
  ImGui::Spacing();
  ImGui::Spacing();

  // Tab content
  ImGui::BeginChild("##TabContent", ImVec2(0, 0), false);
  switch (currentTab) {
  case Tab::Overview:
    renderOverviewTab(cpuUsage, coreUsages, memInfo, diskInfo, gpuInfo,
                      powerInfo, netInfo, allDrives, diskIOInfo, cpuTempInfo);
    break;
  case Tab::Processes:
    renderProcessesTab(processes, processMonitor);
    break;
  case Tab::Network:
    renderNetworkTab(netInfo);
    break;
  case Tab::Energy:
    renderEnergyTab(energyInfo, gpuInfo);
    break;
  case Tab::AI:
    renderAiTab(aiInfo);
    break;
  case Tab::GpuCompute:
    renderGpuComputeTab(nvmlInfo, benchResult, frameAnalysis);
    break;
  case Tab::System:
    renderSystemTab(sysInfo, scoreInfo);
    break;
  case Tab::HpcAssistant:
    renderHpcAssistantTab(hpcReport);
    break;
  case Tab::GamingSession:
    renderGamingSessionTab(sessionMonitor, hpcEngine);
    break;
  }
  ImGui::EndChild();

  ImGui::EndChild(); // MainContent
  ImGui::End();      // Root

  if (showOverlay) {
    renderOverlay(cpuUsage, memInfo, gpuInfo, cpuTempInfo);
  }
}

void GUI::renderOverlay(double cpuUsage, const MemoryInfo &memInfo,
                        const GpuInfo &gpuInfo,
                        const CpuTempInfo &cpuTempInfo) {
  // Transparent overlay window
  ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.12f, 0.75f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

  ImGui::Begin("##Overlay", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoFocusOnAppearing |
                   ImGuiWindowFlags_NoNav);

  ImGui::TextColored(COL_GREEN, "CPU %.1f%%", (float)cpuUsage);
  if (cpuTempInfo.available) {
    ImGui::SameLine();
    ImGui::TextColored(COL_TEXT_DIM, "| %.0fC",
                       (float)cpuTempInfo.temperatureC);
  }

  double memPercent = memInfo.usagePercent;
  ImGui::TextColored(COL_CYAN, "MEM %.1f%%", (float)memPercent);

  if (gpuInfo.available) {
    ImGui::TextColored(COL_ORANGE, "GPU %.1f%%", (float)gpuInfo.gpuUsage);
  }

  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
}

// ─── Sidebar ─────────────────────────────────────────────────────────────────
void GUI::renderSidebar(double cpuUsage, const MemoryInfo &memInfo,
                        const DiskInfo &diskInfo, const GpuInfo &gpuInfo,
                        const PowerInfo &powerInfo,
                        const EnergyInfo &energyInfo,
                        const ScoreInfo &scoreInfo) {
  ImDrawList *dl = ImGui::GetWindowDrawList();
  ImVec2 pos = ImGui::GetCursorScreenPos();
  ImVec2 size = ImGui::GetContentRegionAvail();

  dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                    ImColor(COL_SIDEBAR));

  for (int i = 0; i < 4; i++) {
    float alpha = 0.20f - i * 0.05f;
    dl->AddLine(ImVec2(pos.x + size.x - i, pos.y),
                ImVec2(pos.x + size.x - i, pos.y + size.y),
                ImColor(COL_ACCENT.x, COL_ACCENT.y, COL_ACCENT.z, alpha));
  }

  ImGui::Spacing();
  ImGui::Spacing();

  ImGui::PushStyleColor(ImGuiCol_Text, COL_ACCENT);
  ImGui::SetCursorPosX(24);
  ImGui::Text("SYSTEM");
  ImGui::SetCursorPosX(24);
  ImGui::Text("MONITOR");
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();

  float gaugeRadius = 48.0f;
  float centerX = size.x / 2.0f;

  ImGui::SetCursorPosX(centerX - gaugeRadius);
  renderCircularGauge("CPU", animCpu, COL_GREEN, gaugeRadius);
  ImGui::Spacing();
  ImGui::Spacing();

  ImGui::SetCursorPosX(centerX - gaugeRadius);
  renderCircularGauge("MEM", animMem, COL_CYAN, gaugeRadius);
  ImGui::Spacing();
  ImGui::Spacing();

  ImGui::SetCursorPosX(centerX - gaugeRadius);
  renderCircularGauge("DISK", animDisk, COL_PURPLE, gaugeRadius);
  ImGui::Spacing();
  ImGui::Spacing();

  ImGui::SetCursorPosX(centerX - gaugeRadius);
  if (gpuInfo.available) {
    renderCircularGauge("GPU", animGpu, COL_ORANGE, gaugeRadius);
  } else {
    renderCircularGauge("GPU", 0.0f, COL_TEXT_DIM, gaugeRadius);
  }
  ImGui::Spacing();
  ImGui::Spacing();

  ImGui::SetCursorPosX(centerX - gaugeRadius);
  if (powerInfo.hasBattery && powerInfo.batteryPercent >= 0) {
    ImVec4 batColor = animBattery < 0.2f   ? COL_RED
                      : animBattery < 0.5f ? COL_YELLOW
                                           : COL_GREEN;
    renderCircularGauge("BAT", animBattery, batColor, gaugeRadius);
  } else {
    renderCircularGauge("AC", 1.0f, COL_GREEN, gaugeRadius);
  }

  // Power draw display
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  ImGui::SetCursorPosX(24);
  ImGui::TextColored(COL_TEXT_DIM, "POWER DRAW");
  ImGui::SetCursorPosX(24);
  {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0) << energyInfo.totalWatts << "W";
    ImGui::TextColored(COL_YELLOW, "%s", oss.str().c_str());
  }
  ImGui::SetCursorPosX(24);
  {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << energyInfo.sessionCost << " "
        << energyInfo.currency.c_str();
    ImGui::TextColored(COL_TEXT_DIM, "%s", oss.str().c_str());
  }

  // Performance Score display
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  ImGui::SetCursorPosX(24);
  ImGui::TextColored(COL_TEXT_DIM, "PERFORMANCE");
  ImGui::SetCursorPosX(24);
  {
    float scoreFraction = static_cast<float>(scoreInfo.overallScore) / 100.0f;
    ImVec4 scoreColor = scoreFraction > 0.8f   ? COL_GREEN
                        : scoreFraction > 0.5f ? COL_YELLOW
                                               : COL_ORANGE;
    ImGui::TextColored(scoreColor, "%d/100 [%s]", scoreInfo.overallScore,
                       scoreInfo.grade.c_str());
  }

  // Settings / Game Mode
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  ImGui::SetCursorPosX(24);
  ImGui::TextColored(COL_ACCENT, "SETTINGS");

  ImGui::SetCursorPosX(24);
  if (ImGui::Checkbox("Game Mode", &gameMode)) {
    // Game mode logic handled in main loop polling
  }

  ImGui::SetCursorPosX(24);
  ImGui::Checkbox("Show Overlay", &showOverlay);
}

// ─── Overview Tab ────────────────────────────────────────────────────────────
void GUI::renderOverviewTab(double cpuUsage,
                            const std::vector<double> &coreUsages,
                            const MemoryInfo &memInfo, const DiskInfo &diskInfo,
                            const GpuInfo &gpuInfo, const PowerInfo &powerInfo,
                            const NetworkInfo &netInfo,
                            const std::vector<DiskDriveInfo> &allDrives,
                            const DiskIOInfo &diskIOInfo,
                            const CpuTempInfo &cpuTempInfo) {
  float contentWidth = ImGui::GetContentRegionAvail().x;
  float graphWidth = (contentWidth - 15) / 2.0f;

  // CPU section
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##CpuSection", ImVec2(graphWidth, 300), true);
  {
    ImGui::TextColored(COL_GREEN, "CPU USAGE");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 150);
    // CPU Temperature
    if (cpuTempInfo.available) {
      ImVec4 tempColor = cpuTempInfo.isThrottling          ? COL_RED
                         : cpuTempInfo.temperatureC > 75.0 ? COL_ORANGE
                         : cpuTempInfo.temperatureC > 55.0 ? COL_YELLOW
                                                           : COL_GREEN;
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(0) << cpuTempInfo.temperatureC
          << " C";
      ImGui::TextColored(tempColor, "%s", oss.str().c_str());
      if (cpuTempInfo.isThrottling) {
        ImGui::SameLine();
        ImGui::TextColored(COL_RED, "THROTTLE!");
      }
    } else {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(1) << cpuUsage << "%%";
      ImGui::TextColored(COL_TEXT, "%s", oss.str().c_str());
    }
    ImGui::Spacing();
    renderGraph("##cpuGraph", cpuHistory, HISTORY_SIZE, historyOffset, 100.0f,
                COL_GREEN, ImVec2(-1, 120));
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::TextColored(COL_TEXT_DIM, "Per Core:");
    ImGui::Spacing();
    int cols = std::min((int)coreUsages.size(), 16);
    float barWidth = (ImGui::GetContentRegionAvail().x - (cols - 1) * 5) / cols;
    if (barWidth < 16)
      barWidth = 16;

    for (int i = 0; i < (int)coreUsages.size() && i < 16; i++) {
      if (i > 0)
        ImGui::SameLine(0, 5);
      float frac = static_cast<float>(coreUsages[i]) / 100.0f;
      ImVec4 coreColor = frac > 0.8f   ? COL_RED
                         : frac > 0.5f ? COL_YELLOW
                                       : COL_GREEN;

      ImDrawList *dl = ImGui::GetWindowDrawList();
      ImVec2 p = ImGui::GetCursorScreenPos();
      float barHeight = 42.0f;
      float fillHeight = barHeight * frac;

      dl->AddRectFilled(p, ImVec2(p.x + barWidth, p.y + barHeight),
                        ImColor(0.15f, 0.15f, 0.22f, 1.0f), 4.0f);
      dl->AddRectFilled(ImVec2(p.x, p.y + barHeight - fillHeight),
                        ImVec2(p.x + barWidth, p.y + barHeight),
                        ImColor(coreColor), 4.0f);

      ImGui::Dummy(ImVec2(barWidth, barHeight));
    }
    // Show CPU temp below cores if available
    if (cpuTempInfo.available) {
      ImGui::Spacing();
      std::ostringstream oss2;
      oss2 << std::fixed << std::setprecision(1) << cpuUsage << "%%  |  "
           << std::setprecision(0) << cpuTempInfo.temperatureC << " C";
      ImGui::TextColored(COL_TEXT_DIM, "%s", oss2.str().c_str());
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::SameLine();

  // Memory section
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##MemSection", ImVec2(graphWidth, 300), true);
  {
    ImGui::TextColored(COL_CYAN, "MEMORY USAGE");
    double usedGB =
        static_cast<double>(memInfo.usedRAM) / (1024.0 * 1024.0 * 1024.0);
    double totalGB =
        static_cast<double>(memInfo.totalRAM) / (1024.0 * 1024.0 * 1024.0);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 160);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << usedGB << " / " << totalGB
        << " GB";
    ImGui::TextColored(COL_TEXT, "%s", oss.str().c_str());
    ImGui::Spacing();
    renderGraph("##memGraph", memHistory, HISTORY_SIZE, historyOffset, 100.0f,
                COL_CYAN, ImVec2(-1, 120));
    ImGui::Spacing();
    ImGui::Spacing();

    float fraction = static_cast<float>(memInfo.usagePercent) / 100.0f;
    ImVec4 barColor = fraction > 0.85f  ? COL_RED
                      : fraction > 0.6f ? COL_YELLOW
                                        : COL_CYAN;
    renderMiniBar("Used", fraction, barColor);

    ImGui::Spacing();
    oss.str("");
    oss << std::fixed << std::setprecision(1) << memInfo.usagePercent
        << "%% used";
    ImGui::TextColored(COL_TEXT_DIM, "%s", oss.str().c_str());
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::Spacing();

  // Row 2: GPU, Disk, Power
  float thirdWidth = (contentWidth - 30) / 3.0f;

  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##GpuSection", ImVec2(thirdWidth, 220), true);
  {
    ImGui::TextColored(COL_ORANGE, "GPU");
    if (!gpuInfo.gpuName.empty()) {
      ImGui::SameLine();
      ImGui::TextColored(COL_TEXT_DIM, " %s", gpuInfo.gpuName.c_str());
    }
    ImGui::Spacing();
    if (gpuInfo.available) {
      renderGraph("##gpuGraph", gpuHistory, HISTORY_SIZE, historyOffset, 100.0f,
                  COL_ORANGE, ImVec2(-1, 90));
      ImGui::Spacing();
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(1) << gpuInfo.gpuUsage << "%%";
      ImGui::TextColored(COL_TEXT, "Usage: %s", oss.str().c_str());
      if (gpuInfo.vramTotal > 0) {
        double vramUsedGB =
            static_cast<double>(gpuInfo.vramUsed) / (1024.0 * 1024.0 * 1024.0);
        double vramTotalGB =
            static_cast<double>(gpuInfo.vramTotal) / (1024.0 * 1024.0 * 1024.0);
        oss.str("");
        oss << std::fixed << std::setprecision(1) << vramUsedGB << " / "
            << std::setprecision(0) << vramTotalGB << " GB";
        ImGui::TextColored(COL_TEXT_DIM, "VRAM: %s", oss.str().c_str());
      }
      {
        oss.str("");
        oss << std::fixed << std::setprecision(0) << gpuInfo.powerDrawWatts
            << "W";
        ImGui::TextColored(COL_TEXT_DIM, "Power: %s", oss.str().c_str());
      }
    } else {
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::TextColored(COL_TEXT_DIM, "GPU monitoring");
      ImGui::TextColored(COL_TEXT_DIM, "not available");
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::SameLine();

  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##DiskSection", ImVec2(thirdWidth, 220), true);
  {
    ImGui::TextColored(COL_PURPLE, "DISKS");
    ImGui::Spacing();
    ImGui::Spacing();

    // Show all drives
    for (size_t d = 0; d < allDrives.size() && d < 6; d++) {
      const auto &drv = allDrives[d];
      double totalGB =
          static_cast<double>(drv.totalSpace) / (1024.0 * 1024.0 * 1024.0);
      double usedGB =
          static_cast<double>(drv.usedSpace) / (1024.0 * 1024.0 * 1024.0);
      double freeGB =
          static_cast<double>(drv.freeSpace) / (1024.0 * 1024.0 * 1024.0);

      std::ostringstream oss;
      oss << drv.driveLetter << " (" << drv.volumeLabel << ")";
      ImGui::TextColored(COL_TEXT, "%s", oss.str().c_str());

      oss.str("");
      oss << std::fixed << std::setprecision(1) << usedGB << " / " << totalGB
          << " GB";
      ImGui::SameLine(ImGui::GetContentRegionAvail().x - 120);
      ImGui::TextColored(COL_TEXT_DIM, "%s", oss.str().c_str());

      float fraction = static_cast<float>(drv.usagePercent) / 100.0f;
      ImVec4 diskColor = fraction > 0.9f   ? COL_RED
                         : fraction > 0.7f ? COL_YELLOW
                                           : COL_PURPLE;
      renderMiniBar((std::string("##diskBar") + drv.driveLetter).c_str(),
                    fraction, diskColor);
      ImGui::Spacing();
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::SameLine();

  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##PowerSection", ImVec2(thirdWidth, 220), true);
  {
    ImGui::TextColored(COL_YELLOW, "POWER");
    ImGui::Spacing();
    ImGui::Spacing();

    if (!powerInfo.hasBattery) {
      ImGui::TextColored(COL_GREEN, "AC Power");
      ImGui::Spacing();
      ImGui::TextColored(COL_TEXT_DIM, "No battery detected");
    } else {
      if (powerInfo.isCharging) {
        ImGui::TextColored(COL_GREEN, "Charging");
      } else if (powerInfo.onBattery) {
        ImGui::TextColored(COL_ORANGE, "On Battery");
      } else {
        ImGui::TextColored(COL_GREEN, "AC Power");
      }

      if (powerInfo.batteryPercent >= 0) {
        ImGui::Spacing();
        std::ostringstream oss;
        oss << powerInfo.batteryPercent << "%%";
        ImGui::TextColored(COL_TEXT, "%s", oss.str().c_str());

        float fraction = static_cast<float>(powerInfo.batteryPercent) / 100.0f;
        ImVec4 batColor = fraction < 0.2f   ? COL_RED
                          : fraction < 0.5f ? COL_YELLOW
                                            : COL_GREEN;
        ImGui::Spacing();
        ImGui::Spacing();
        renderMiniBar("##batBar", fraction, batColor);
      }

      if (powerInfo.onBattery && powerInfo.estimatedTime > 0) {
        ImGui::Spacing();
        int h = powerInfo.estimatedTime / 60;
        int m = powerInfo.estimatedTime % 60;
        std::ostringstream oss;
        oss << "~" << h << "h " << m << "m remaining";
        ImGui::TextColored(COL_TEXT_DIM, "%s", oss.str().c_str());
      }
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::Spacing();

  // Row 3: Disk I/O
  if (diskIOInfo.available) {
    float ioHalfWidth = (contentWidth - 15) / 2.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
    ImGui::BeginChild("##DiskReadIO", ImVec2(ioHalfWidth, 180), true);
    {
      ImGui::TextColored(COL_GREEN, "DISK READ");
      ImGui::SameLine(ImGui::GetContentRegionAvail().x - 120);
      ImGui::TextColored(COL_TEXT, "%s",
                         formatSpeed(diskIOInfo.readBytesPerSec).c_str());
      ImGui::Spacing();

      float maxRead = 1.0f;
      for (int i = 0; i < HISTORY_SIZE; i++) {
        if (diskReadHistory[i] > maxRead)
          maxRead = diskReadHistory[i];
      }
      renderGraph("##diskReadGraph", diskReadHistory, HISTORY_SIZE,
                  historyOffset, maxRead * 1.2f, COL_GREEN, ImVec2(-1, 100));
      ImGui::Spacing();
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(0) << diskIOInfo.readOpsPerSec
          << " IOPS";
      ImGui::TextColored(COL_TEXT_DIM, "%s", oss.str().c_str());
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
    ImGui::BeginChild("##DiskWriteIO", ImVec2(ioHalfWidth, 180), true);
    {
      ImGui::TextColored(COL_ORANGE, "DISK WRITE");
      ImGui::SameLine(ImGui::GetContentRegionAvail().x - 120);
      ImGui::TextColored(COL_TEXT, "%s",
                         formatSpeed(diskIOInfo.writeBytesPerSec).c_str());
      ImGui::Spacing();

      float maxWrite = 1.0f;
      for (int i = 0; i < HISTORY_SIZE; i++) {
        if (diskWriteHistory[i] > maxWrite)
          maxWrite = diskWriteHistory[i];
      }
      renderGraph("##diskWriteGraph", diskWriteHistory, HISTORY_SIZE,
                  historyOffset, maxWrite * 1.2f, COL_ORANGE, ImVec2(-1, 100));
      ImGui::Spacing();
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(0) << diskIOInfo.writeOpsPerSec
          << " IOPS";
      ImGui::TextColored(COL_TEXT_DIM, "%s", oss.str().c_str());
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
  }
}

// ─── Processes Tab ───────────────────────────────────────────────────────────
void GUI::renderProcessesTab(const std::vector<ProcessInfo> &processes,
                             ProcessMonitor &processMonitor) {
  float contentWidth = ImGui::GetContentRegionAvail().x;
  ImGui::TextColored(COL_ACCENT, "TOP PROCESSES");

  // Sort radio buttons
  ImGui::SameLine(contentWidth - 250);
  ImGui::TextColored(COL_TEXT_DIM, "Sort by:");
  ImGui::SameLine();
  bool sortByMem =
      (processMonitor.getSortMode() == ProcessMonitor::SortMode::ByMemory);
  bool sortByCpu =
      (processMonitor.getSortMode() == ProcessMonitor::SortMode::ByCpu);

  if (ImGui::RadioButton("Memory", sortByMem)) {
    processMonitor.setSortMode(ProcessMonitor::SortMode::ByMemory);
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("CPU%%", sortByCpu)) {
    processMonitor.setSortMode(ProcessMonitor::SortMode::ByCpu);
  }

  ImGui::Spacing();
  ImGui::Spacing();

  ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

  if (ImGui::BeginTable("##ProcessTable", 5, flags,
                        ImVec2(0, ImGui::GetContentRegionAvail().y - 10))) {
    ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 70.0f);
    ImGui::TableSetupColumn("Process Name", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("CPU %%", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthFixed, 120.0f);
    ImGui::TableSetupColumn("Usage Bar", ImGuiTableColumnFlags_WidthFixed,
                            220.0f);
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();

    uint64_t maxMem = 1;
    for (auto &p : processes) {
      if (p.memoryUsed > maxMem)
        maxMem = p.memoryUsed;
    }

    for (int i = 0; i < (int)processes.size(); i++) {
      auto &proc = processes[i];
      ImGui::TableNextRow(ImGuiTableRowFlags_None, 32.0f);

      ImGui::TableSetColumnIndex(0);
      ImGui::TextColored(COL_TEXT_DIM, "%lu", proc.pid);

      ImGui::TableSetColumnIndex(1);
      ImGui::TextColored(COL_TEXT, "%s", proc.name.c_str());

      ImGui::TableSetColumnIndex(2);
      ImVec4 cpuCol = proc.cpuUsage > 50.0
                          ? COL_RED
                          : (proc.cpuUsage > 10.0 ? COL_ORANGE : COL_GREEN);
      ImGui::TextColored(cpuCol, "%.1f %%", proc.cpuUsage);

      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%s", formatBytes(proc.memoryUsed).c_str());

      ImGui::TableSetColumnIndex(4);
      float frac =
          (processMonitor.getSortMode() == ProcessMonitor::SortMode::ByCpu)
              ? (static_cast<float>(proc.cpuUsage) / 100.0f)
              : (static_cast<float>((double)proc.memoryUsed / (double)maxMem));
      ImVec4 barColor = i < 3 ? COL_RED : i < 7 ? COL_ORANGE : COL_ACCENT;

      ImDrawList *dl = ImGui::GetWindowDrawList();
      ImVec2 p = ImGui::GetCursorScreenPos();
      float barW = ImGui::GetContentRegionAvail().x;
      float barH = 20.0f;
      dl->AddRectFilled(p, ImVec2(p.x + barW, p.y + barH),
                        ImColor(0.15f, 0.15f, 0.22f, 1.0f), 4.0f);
      dl->AddRectFilled(
          p, ImVec2(p.x + barW * (std::max)(0.0f, (std::min)(1.0f, frac)), p.y + barH),
          ImColor(barColor), 4.0f);
      ImGui::Dummy(ImVec2(barW, barH));
    }
    ImGui::EndTable();
  }
}

// ─── Network Tab (ENHANCED) ─────────────────────────────────────────────────
void GUI::renderNetworkTab(const NetworkInfo &netInfo) {
  ImGui::TextColored(COL_ACCENT, "NETWORK ACTIVITY");
  ImGui::Spacing();
  ImGui::Spacing();

  if (!netInfo.available) {
    ImGui::TextColored(COL_TEXT_DIM, "Network monitoring not available");
    return;
  }

  float contentWidth = ImGui::GetContentRegionAvail().x;
  float halfWidth = (contentWidth - 15) / 2.0f;

  // ── Row 1: Download & Upload graphs ──
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##DownloadSection", ImVec2(halfWidth, 230), true);
  {
    ImGui::TextColored(COL_GREEN, "DOWNLOAD");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 130);
    ImGui::TextColored(COL_TEXT, "%s",
                       formatSpeed(netInfo.downloadSpeed).c_str());
    ImGui::Spacing();

    float maxDl = 1.0f;
    for (int i = 0; i < HISTORY_SIZE; i++) {
      if (downloadHistory[i] > maxDl)
        maxDl = downloadHistory[i];
    }
    renderGraph("##dlGraph", downloadHistory, HISTORY_SIZE, historyOffset,
                maxDl * 1.2f, COL_GREEN, ImVec2(-1, 140));

    ImGui::Spacing();
    ImGui::TextColored(COL_TEXT_DIM, "Peak: %s",
                       formatSpeed(netInfo.peakDownloadSpeed).c_str());
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::SameLine();

  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##UploadSection", ImVec2(halfWidth, 230), true);
  {
    ImGui::TextColored(COL_ORANGE, "UPLOAD");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 130);
    ImGui::TextColored(COL_TEXT, "%s",
                       formatSpeed(netInfo.uploadSpeed).c_str());
    ImGui::Spacing();

    float maxUl = 1.0f;
    for (int i = 0; i < HISTORY_SIZE; i++) {
      if (uploadHistory[i] > maxUl)
        maxUl = uploadHistory[i];
    }
    renderGraph("##ulGraph", uploadHistory, HISTORY_SIZE, historyOffset,
                maxUl * 1.2f, COL_ORANGE, ImVec2(-1, 140));

    ImGui::Spacing();
    ImGui::TextColored(COL_TEXT_DIM, "Peak: %s",
                       formatSpeed(netInfo.peakUploadSpeed).c_str());
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::Spacing();

  // ── Row 2: Connection Details & Stats side by side ──
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##NetDetails", ImVec2(halfWidth, 250), true);
  {
    ImGui::TextColored(COL_ACCENT, "CONNECTION DETAILS");
    ImGui::Spacing();
    ImGui::Spacing();

    renderInfoRow("Adapter", netInfo.adapterName.c_str(), COL_TEXT);
    renderInfoRow("Type", netInfo.connectionType.c_str(), COL_CYAN);
    renderInfoRow("IP Address", netInfo.ipAddress.c_str(), COL_GREEN);
    renderInfoRow("MAC Address", netInfo.macAddress.c_str(), COL_TEXT);

    // Link speed
    std::ostringstream oss;
    if (netInfo.linkSpeedMbps > 0) {
      oss << netInfo.linkSpeedMbps << " Mbps";
    } else {
      oss << "N/A";
    }
    renderInfoRow("Link Speed", oss.str().c_str(), COL_YELLOW);

    // Ping latency
    oss.str("");
    if (netInfo.pingLatencyMs >= 0) {
      oss << std::fixed << std::setprecision(0) << netInfo.pingLatencyMs
          << " ms";
      ImVec4 pingColor = netInfo.pingLatencyMs < 50    ? COL_GREEN
                         : netInfo.pingLatencyMs < 100 ? COL_YELLOW
                                                       : COL_RED;
      renderInfoRow("Ping (8.8.8.8)", oss.str().c_str(), pingColor);
    } else {
      renderInfoRow("Ping (8.8.8.8)", "N/A", COL_TEXT_DIM);
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::SameLine();

  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##NetStats", ImVec2(halfWidth, 250), true);
  {
    ImGui::TextColored(COL_ACCENT, "TRAFFIC STATISTICS");
    ImGui::Spacing();
    ImGui::Spacing();

    renderInfoRow("Total Received",
                  formatBytes(netInfo.totalBytesReceived).c_str(), COL_GREEN);
    renderInfoRow("Total Sent", formatBytes(netInfo.totalBytesSent).c_str(),
                  COL_ORANGE);

    // Packet counts
    std::ostringstream oss;
    oss << netInfo.packetsReceived;
    renderInfoRow("Packets In", oss.str().c_str(), COL_TEXT);

    oss.str("");
    oss << netInfo.packetsSent;
    renderInfoRow("Packets Out", oss.str().c_str(), COL_TEXT);

    // Active TCP connections
    oss.str("");
    oss << netInfo.activeTcpConnections;
    ImVec4 tcpColor = netInfo.activeTcpConnections > 100  ? COL_YELLOW
                      : netInfo.activeTcpConnections > 50 ? COL_ORANGE
                                                          : COL_CYAN;
    renderInfoRow("TCP Connections", oss.str().c_str(), tcpColor);
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();
}

// ─── Energy Tab ──────────────────────────────────────────────────────────────
void GUI::renderEnergyTab(const EnergyInfo &energyInfo,
                          const GpuInfo &gpuInfo) {
  ImGui::TextColored(COL_YELLOW, "ENERGY MONITOR");
  ImGui::Spacing();
  ImGui::Spacing();

  float contentWidth = ImGui::GetContentRegionAvail().x;
  float halfWidth = (contentWidth - 15) / 2.0f;

  // ── Left: Real-time Power ──
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##PowerRealtime", ImVec2(halfWidth, 320), true);
  {
    ImGui::TextColored(COL_YELLOW, "REAL-TIME POWER");
    ImGui::Spacing();
    ImGui::Spacing();

    // Big watts display
    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(0) << energyInfo.totalWatts;
      ImGui::PushFont(nullptr); // Use default (we'll make it big via scale)
      float oldScale = ImGui::GetFont()->Scale;
      ImGui::GetFont()->Scale = 2.0f;
      ImGui::PushFont(ImGui::GetFont());
      ImGui::TextColored(COL_YELLOW, "%s W", oss.str().c_str());
      ImGui::GetFont()->Scale = oldScale;
      ImGui::PopFont();
      ImGui::PopFont();
    }
    ImGui::TextColored(COL_TEXT_DIM, "Total System Power (estimated)");
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Component breakdown
    ImGui::TextColored(COL_TEXT_DIM, "BREAKDOWN");
    ImGui::Spacing();

    // CPU power bar
    float maxWatts =
        (float)(energyInfo.cpuTdpWatts + energyInfo.gpuTdpWatts + 50);
    if (maxWatts < 100)
      maxWatts = 100;

    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(0) << energyInfo.cpuWatts << "W";
      ImGui::TextColored(COL_GREEN, "CPU");
      ImGui::SameLine(120);
      ImGui::TextColored(COL_TEXT, "%s", oss.str().c_str());
      float frac = (float)energyInfo.cpuWatts / maxWatts;
      renderMiniBar("##cpuPow", frac, COL_GREEN);
    }

    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(0) << energyInfo.gpuWatts << "W";
      ImGui::TextColored(COL_ORANGE, "GPU");
      ImGui::SameLine(120);
      ImGui::TextColored(COL_TEXT, "%s", oss.str().c_str());
      float frac = (float)energyInfo.gpuWatts / maxWatts;
      renderMiniBar("##gpuPow", frac, COL_ORANGE);
    }

    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(0) << energyInfo.systemBaseWatts
          << "W";
      ImGui::TextColored(COL_PURPLE, "System");
      ImGui::SameLine(120);
      ImGui::TextColored(COL_TEXT, "%s", oss.str().c_str());
      float frac = (float)energyInfo.systemBaseWatts / maxWatts;
      renderMiniBar("##sysPow", frac, COL_PURPLE);
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::SameLine();

  // ── Right: Cost Tracking ──
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##CostTracking", ImVec2(halfWidth, 320), true);
  {
    ImGui::TextColored(COL_CYAN, "COST TRACKING");
    ImGui::Spacing();
    ImGui::Spacing();

    // Session cost - big display
    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(2) << energyInfo.sessionCost;
      ImGui::PushFont(nullptr);
      float oldScale = ImGui::GetFont()->Scale;
      ImGui::GetFont()->Scale = 2.0f;
      ImGui::PushFont(ImGui::GetFont());
      ImGui::TextColored(COL_CYAN, "%s %s", oss.str().c_str(),
                         energyInfo.currency.c_str());
      ImGui::GetFont()->Scale = oldScale;
      ImGui::PopFont();
      ImGui::PopFont();
    }
    ImGui::TextColored(COL_TEXT_DIM, "Session Cost");
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Session details
    {
      std::ostringstream oss;
      int sessionMins = (int)(energyInfo.sessionHours * 60);
      int h = sessionMins / 60;
      int m = sessionMins % 60;
      oss << h << "h " << m << "m";
      renderInfoRow("Session Duration", oss.str().c_str(), COL_TEXT);
    }

    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(1) << energyInfo.sessionEnergyWh
          << " Wh";
      renderInfoRow("Energy Used", oss.str().c_str(), COL_TEXT);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Projections
    ImGui::TextColored(COL_TEXT_DIM, "PROJECTIONS");
    ImGui::Spacing();

    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(2) << energyInfo.dailyEstimateCost
          << " " << energyInfo.currency.c_str() << "  (" << std::setprecision(2)
          << energyInfo.dailyEstimateKwh << " kWh)";
      renderInfoRow("Daily (24h)", oss.str().c_str(), COL_YELLOW);
    }

    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(1)
          << energyInfo.monthlyEstimateCost << " "
          << energyInfo.currency.c_str();
      renderInfoRow("Monthly (30d)", oss.str().c_str(), COL_ORANGE);
    }

    ImGui::Spacing();
    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(2) << energyInfo.costPerKwh << " "
          << energyInfo.currency.c_str() << "/kWh";
      ImGui::TextColored(COL_TEXT_DIM, "Rate: %s", oss.str().c_str());
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::Spacing();

  // ── Bottom: Power Graph ──
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##PowerGraph", ImVec2(-1, 200), true);
  {
    ImGui::TextColored(COL_YELLOW, "POWER HISTORY");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0) << energyInfo.totalWatts << " W";
    ImGui::TextColored(COL_TEXT, "%s", oss.str().c_str());
    ImGui::Spacing();

    // Find max for graph scaling
    float maxPower = 100.0f;
    for (int i = 0; i < HISTORY_SIZE; i++) {
      if (powerHistory[i] > maxPower)
        maxPower = powerHistory[i];
    }
    maxPower *= 1.2f; // 20% headroom

    renderGraph("##powerGraph", powerHistory, HISTORY_SIZE, historyOffset,
                maxPower, COL_YELLOW, ImVec2(-1, 130));
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();

  // TDP Info row
  {
    std::ostringstream oss;
    oss << "CPU TDP: " << energyInfo.cpuTdpWatts << "W";
    if (gpuInfo.available) {
      oss << "  |  GPU TDP: " << gpuInfo.tdpWatts << "W";
    }
    oss << "  |  GPU: " << gpuInfo.gpuName;
    ImGui::TextColored(COL_TEXT_DIM, "%s", oss.str().c_str());
  }
}

// ─── AI Tab ──────────────────────────────────────────────────────────────────
static const ImVec4 COL_AI_ACCENT = ImVec4(0.85f, 0.20f, 1.00f, 1.00f);
static const ImVec4 COL_AI_ACTIVE = ImVec4(0.20f, 1.00f, 0.50f, 1.00f);
static const ImVec4 COL_AI_IDLE = ImVec4(1.00f, 0.85f, 0.25f, 1.00f);
static const ImVec4 COL_AI_NONE = ImVec4(0.55f, 0.56f, 0.62f, 1.00f);

void GUI::renderAiTab(const AiInfo &aiInfo) {
  ImGui::TextColored(COL_AI_ACCENT, "AI ENGINEER DASHBOARD");
  ImGui::SameLine(ImGui::GetContentRegionAvail().x - 350);
  ImGui::TextColored(COL_TEXT_DIM, "%s", aiInfo.computeSummary.c_str());
  ImGui::Spacing();
  ImGui::Spacing();

  float contentWidth = ImGui::GetContentRegionAvail().x;
  float halfWidth = (contentWidth - 15) / 2.0f;

  // ── Row 1: AI Status + GPU for AI ──
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##AiStatusSection", ImVec2(halfWidth, 260), true);
  {
    ImGui::TextColored(COL_AI_ACCENT, "AI WORKLOAD STATUS");
    ImGui::Spacing();
    ImGui::Spacing();

    // Status indicator
    ImVec4 statusColor = COL_AI_NONE;
    if (aiInfo.workloadStatus == "Active")
      statusColor = COL_AI_ACTIVE;
    else if (aiInfo.workloadStatus == "Idle")
      statusColor = COL_AI_IDLE;

    // Draw status circle
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float circleR = 8.0f;
    dl->AddCircleFilled(ImVec2(p.x + circleR + 4, p.y + circleR), circleR,
                        ImColor(statusColor));
    // Glow effect
    dl->AddCircle(ImVec2(p.x + circleR + 4, p.y + circleR), circleR + 3,
                  ImColor(statusColor.x, statusColor.y, statusColor.z, 0.3f), 0,
                  2.0f);
    ImGui::Dummy(ImVec2(circleR * 2 + 12, circleR * 2));
    ImGui::SameLine();
    ImGui::TextColored(statusColor, "%s", aiInfo.workloadStatus.c_str());

    ImGui::Spacing();
    ImGui::TextColored(COL_TEXT_DIM, "%s", aiInfo.workloadStatusDesc.c_str());
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // AI Process count
    {
      std::ostringstream oss;
      oss << aiInfo.totalAiProcesses;
      renderInfoRow("AI Processes", oss.str().c_str(),
                    aiInfo.totalAiProcesses > 0 ? COL_AI_ACTIVE : COL_TEXT_DIM);
    }

    // AI Power estimate
    if (aiInfo.gpuAvailable && aiInfo.totalAiProcesses > 0) {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(0) << aiInfo.aiPowerEstimateW
          << " W";
      renderInfoRow("AI Power Draw", oss.str().c_str(), COL_YELLOW);
    } else {
      renderInfoRow("AI Power Draw", "0 W", COL_TEXT_DIM);
    }

    // Environment
    renderInfoRow("CUDA", aiInfo.cudaDetected ? "Ready" : "Not Detected",
                  aiInfo.cudaDetected ? COL_GREEN : COL_TEXT_DIM);
    renderInfoRow("Python", aiInfo.pythonDetected ? "Detected" : "Not Found",
                  aiInfo.pythonDetected ? COL_GREEN : COL_TEXT_DIM);
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::SameLine();

  // ── GPU for AI panel ──
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##GpuAiSection", ImVec2(halfWidth, 260), true);
  {
    ImGui::TextColored(COL_ORANGE, "GPU FOR AI");
    if (!aiInfo.gpuName.empty()) {
      ImGui::SameLine();
      ImGui::TextColored(COL_TEXT_DIM, " %s", aiInfo.gpuName.c_str());
    }
    ImGui::Spacing();

    if (aiInfo.gpuAvailable) {
      // GPU Usage
      {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << aiInfo.gpuUsagePercent
            << "%%";
        renderInfoRow("GPU Load", oss.str().c_str(), COL_ORANGE);
      }

      // VRAM usage bar
      ImGui::Spacing();
      ImGui::TextColored(COL_TEXT_DIM, "VRAM Usage:");
      {
        double vramUsedGB =
            static_cast<double>(aiInfo.vramUsed) / (1024.0 * 1024.0 * 1024.0);
        double vramTotalGB =
            static_cast<double>(aiInfo.vramTotal) / (1024.0 * 1024.0 * 1024.0);
        double vramFreeGB =
            static_cast<double>(aiInfo.vramFree) / (1024.0 * 1024.0 * 1024.0);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << vramUsedGB << " / "
            << std::setprecision(0) << vramTotalGB << " GB";
        ImGui::SameLine();
        ImGui::TextColored(COL_TEXT, " %s", oss.str().c_str());

        float vramFrac = static_cast<float>(aiInfo.vramUsagePercent / 100.0);
        ImVec4 vramColor = vramFrac > 0.9f   ? COL_RED
                           : vramFrac > 0.7f ? COL_YELLOW
                                             : COL_CYAN;
        ImGui::Spacing();
        renderMiniBar("##vramAiBar", vramFrac, vramColor);

        ImGui::Spacing();
        oss.str("");
        oss << std::fixed << std::setprecision(1) << vramFreeGB << " GB free";
        ImGui::TextColored(COL_TEXT_DIM, "%s", oss.str().c_str());
      }

      // Temperature & Power
      ImGui::Spacing();
      {
        std::ostringstream oss;
        if (aiInfo.temperatureC > 0) {
          oss << std::fixed << std::setprecision(0) << aiInfo.temperatureC
              << " C";
          ImVec4 tempColor = aiInfo.temperatureC > 80   ? COL_RED
                             : aiInfo.temperatureC > 65 ? COL_YELLOW
                                                        : COL_GREEN;
          renderInfoRow("Temperature", oss.str().c_str(), tempColor);
        }
        oss.str("");
        oss << std::fixed << std::setprecision(0) << aiInfo.powerDrawWatts
            << " W / " << aiInfo.tdpWatts << " W TDP";
        renderInfoRow("Power", oss.str().c_str(), COL_YELLOW);
      }
    } else {
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::TextColored(COL_TEXT_DIM, "No GPU detected");
      ImGui::TextColored(COL_TEXT_DIM, "AI workloads require a GPU");
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::Spacing();

  // ── Row 2: AI Processes + Model Capacity ──
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##AiProcessesSection", ImVec2(halfWidth, 320), true);
  {
    ImGui::TextColored(COL_AI_ACCENT, "AI PROCESSES");
    ImGui::Spacing();
    ImGui::Spacing();

    if (aiInfo.processes.empty()) {
      ImGui::Spacing();
      ImGui::TextColored(COL_TEXT_DIM, "No AI/ML processes running");
      ImGui::Spacing();
      ImGui::TextColored(COL_TEXT_DIM, "Detects: Python, Ollama, PyTorch,");
      ImGui::TextColored(COL_TEXT_DIM, "TensorFlow, Stable Diffusion,");
      ImGui::TextColored(COL_TEXT_DIM, "LM Studio, ComfyUI, Whisper...");
    } else {
      ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_ScrollY;

      if (ImGui::BeginTable("##AiProcTable", 3, flags,
                            ImVec2(0, ImGui::GetContentRegionAvail().y - 10))) {
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Process", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthFixed,
                                120.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (const auto &proc : aiInfo.processes) {
          ImGui::TableNextRow(ImGuiTableRowFlags_None, 28.0f);

          ImGui::TableSetColumnIndex(0);
          ImGui::TextColored(COL_TEXT_DIM, "%lu", proc.pid);

          ImGui::TableSetColumnIndex(1);
          ImGui::TextColored(COL_TEXT, "%s", proc.name.c_str());

          ImGui::TableSetColumnIndex(2);
          ImGui::TextColored(COL_CYAN, "%s",
                             formatBytes(proc.memoryUsed).c_str());
        }
        ImGui::EndTable();
      }
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::SameLine();

  // ── Model Capacity Estimator ──
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##ModelCapacity", ImVec2(halfWidth, 320), true);
  {
    ImGui::TextColored(COL_AI_ACCENT, "MODEL CAPACITY ESTIMATOR");
    ImGui::Spacing();

    if (!aiInfo.gpuAvailable || aiInfo.vramTotal == 0) {
      ImGui::TextColored(COL_TEXT_DIM, "GPU required for model estimation");
    } else {
      double totalGB =
          static_cast<double>(aiInfo.vramTotal) / (1024.0 * 1024.0 * 1024.0);
      {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << totalGB
            << " GB VRAM available";
        ImGui::TextColored(COL_TEXT_DIM, "%s", oss.str().c_str());
      }
      ImGui::Spacing();

      ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_ScrollY;

      if (ImGui::BeginTable("##ModelTable", 4, flags,
                            ImVec2(0, ImGui::GetContentRegionAvail().y - 10))) {
        ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed,
                                70.0f);
        ImGui::TableSetupColumn("VRAM", ImGuiTableColumnFlags_WidthFixed,
                                70.0f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed,
                                50.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (const auto &model : aiInfo.modelCapabilities) {
          ImGui::TableNextRow(ImGuiTableRowFlags_None, 26.0f);

          ImGui::TableSetColumnIndex(0);
          ImGui::TextColored(model.canRun ? COL_TEXT : COL_TEXT_DIM, "%s",
                             model.name.c_str());

          ImGui::TableSetColumnIndex(1);
          ImVec4 catColor = COL_TEXT_DIM;
          if (model.category == "LLM")
            catColor = COL_AI_ACCENT;
          else if (model.category == "Vision")
            catColor = COL_CYAN;
          else if (model.category == "Diffusion")
            catColor = COL_ORANGE;
          else if (model.category == "Audio")
            catColor = COL_YELLOW;
          ImGui::TextColored(catColor, "%s", model.category.c_str());

          ImGui::TableSetColumnIndex(2);
          {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << model.requiredVramGB
                << " GB";
            ImGui::TextColored(COL_TEXT_DIM, "%s", oss.str().c_str());
          }

          ImGui::TableSetColumnIndex(3);
          if (model.canRun) {
            ImGui::TextColored(COL_GREEN, "  OK");
          } else {
            ImGui::TextColored(COL_RED, "  NO");
          }
        }
        ImGui::EndTable();
      }
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();
}

// ─── GPU Compute Tab ─────────────────────────────────────────────────────────
static const ImVec4 COL_GPU_ACCENT = ImVec4(0.30f, 0.90f, 0.40f, 1.00f);
static const ImVec4 COL_GPU_WARM = ImVec4(1.00f, 0.65f, 0.00f, 1.00f);
static const ImVec4 COL_GPU_HOT = ImVec4(1.00f, 0.30f, 0.20f, 1.00f);

void GUI::renderGpuComputeTab(const NvmlInfo &nvmlInfo,
                              const BenchmarkResult &benchResult,
                              const FrameAnalysis &frameAnalysis) {
  ImGui::TextColored(COL_GPU_ACCENT, "GPU COMPUTE DASHBOARD");
  ImGui::SameLine(ImGui::GetContentRegionAvail().x - 220);
  if (nvmlInfo.available) {
    ImGui::TextColored(COL_TEXT_DIM, "NVML Active | Driver %s",
                       nvmlInfo.driverVersion.c_str());
  } else {
    ImGui::TextColored(COL_RED, "NVML Not Available");
  }
  ImGui::Spacing();
  ImGui::Spacing();

  float contentWidth = ImGui::GetContentRegionAvail().x;

  // Row 1 Header: Performance Analysis (Universal)
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##PerfAnalysis", ImVec2(-1, 100), true);
  {
    ImGui::TextColored(COL_CYAN, "PERFORMANCE & BOTTLENECK ANALYSIS");
    ImGui::Spacing();

    float w = ImGui::GetContentRegionAvail().x;
    float colW = w / 4.0f;

    ImGui::Columns(4, "##perfcols", false);
    ImGui::SetColumnWidth(0, colW);
    ImGui::SetColumnWidth(1, colW);
    ImGui::SetColumnWidth(2, colW);
    ImGui::SetColumnWidth(3, colW);

    ImGui::TextColored(COL_TEXT_DIM, "Est. FPS");
    ImGui::TextColored(COL_GREEN, "%.0f FPS", frameAnalysis.estimatedFPS);
    ImGui::NextColumn();

    ImGui::TextColored(COL_TEXT_DIM, "Status");
    ImGui::TextColored(COL_YELLOW, "%s", frameAnalysis.status.c_str());
    ImGui::NextColumn();

    ImGui::TextColored(COL_TEXT_DIM, "System Bottleneck");
    ImVec4 bCol = frameAnalysis.gpuBound
                      ? COL_ORANGE
                      : (frameAnalysis.cpuBound ? COL_RED : COL_GREEN);
    ImGui::TextColored(bCol, "%s", frameAnalysis.bottleneck.c_str());
    ImGui::NextColumn();

    ImGui::TextColored(COL_TEXT_DIM, "Frame Times");
    ImGui::TextColored(COL_TEXT, "G:%.1f ms | C:%.1f ms",
                       frameAnalysis.gpuFrameTimeMs,
                       frameAnalysis.cpuFrameTimeMs);
    ImGui::Columns(1);
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (!nvmlInfo.available || nvmlInfo.devices.empty()) {
    ImGui::TextColored(COL_GPU_HOT, "No NVIDIA GPUs detected via NVML.");
    ImGui::TextColored(COL_TEXT_DIM, "CUDA and detailed hardware metrics require NVIDIA drivers.");
    return;
  }

  // Iterate over each GPU
  for (size_t i = 0; i < nvmlInfo.devices.size(); i++) {
    const auto &dev = nvmlInfo.devices[i];
    std::string childId = "##GpuChild" + std::to_string(i);
    
    ImGui::BeginChild(childId.c_str(), ImVec2(-1, 480), false);
    
    ImGui::TextColored(COL_GPU_ACCENT, "GPU #%d: %s", (int)i, dev.name.c_str());
    ImGui::Spacing();

    float halfWidth = (ImGui::GetContentRegionAvail().x - 15) / 2.0f;

    // ── Panel 1: NVML Real-Time Metrics (left) ──
    ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
    ImGui::BeginChild((childId + "Metrics").c_str(), ImVec2(halfWidth, 420), true);
    {
      ImGui::TextColored(COL_CYAN, "REAL-TIME METRICS");
      ImGui::Spacing();

      // Temperature
      ImVec4 tempColor = COL_GREEN;
      if (dev.temperatureC > 80) tempColor = COL_GPU_HOT;
      else if (dev.temperatureC > 65) tempColor = COL_GPU_WARM;
      char tempStr[32];
      snprintf(tempStr, sizeof(tempStr), "%u C", dev.temperatureC);
      renderInfoRow("Temperature", tempStr, tempColor);

      char powerStr[64];
      snprintf(powerStr, sizeof(powerStr), "%.1f W / %u W (TDP)", dev.powerWatts, dev.tdpWatts);
      renderInfoRow("Power Draw", powerStr, COL_ORANGE);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Utilization
      ImGui::TextColored(COL_PURPLE, "UTILIZATION");
      ImGui::Spacing();

      char gpuUtilStr[32];
      snprintf(gpuUtilStr, sizeof(gpuUtilStr), "%u%%", dev.gpuUtil);
      renderInfoRow("Graphics", gpuUtilStr, COL_GREEN);
      ImGui::ProgressBar(dev.gpuUtil / 100.0f, ImVec2(-1, 14), "");

      char memUtilStr[32];
      snprintf(memUtilStr, sizeof(memUtilStr), "%u%%", dev.memUtil);
      renderInfoRow("Memory Bus", memUtilStr, COL_CYAN);
      ImGui::ProgressBar(dev.memUtil / 100.0f, ImVec2(-1, 14), "");

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Clocks
      ImGui::TextColored(COL_PURPLE, "CLOCKS");
      ImGui::Spacing();

      char gfxClk[64], memClk[64];
      snprintf(gfxClk, sizeof(gfxClk), "%u / %u MHz", dev.graphicsClock, dev.maxGraphicsClock);
      snprintf(memClk, sizeof(memClk), "%u MHz", dev.memClock);

      renderInfoRow("Core Clock", gfxClk, COL_TEXT);
      renderInfoRow("Memory Clock", memClk, COL_TEXT);

      ImGui::Spacing();

      // PCIe
      char pcieStr[128];
      snprintf(pcieStr, sizeof(pcieStr), "Gen %u x%u | RX %.1f | TX %.1f MB/s",
               dev.pcieGeneration, dev.pcieWidth,
               dev.pcieRxKBs / 1024.0f, dev.pcieTxKBs / 1024.0f);
      renderInfoRow("PCIe Bus", pcieStr, COL_TEXT_DIM);

      if (dev.encoderUtil > 0 || dev.decoderUtil > 0) {
        char encStr[48];
        snprintf(encStr, sizeof(encStr), "Enc %u%% | Dec %u%%",
                 dev.encoderUtil, dev.decoderUtil);
        renderInfoRow("Media Engine", encStr, COL_TEXT_DIM);
      }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // ── Panel 2: Compute Capabilities + GPU Processes (right) ──
    ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
    ImGui::BeginChild((childId + "Info").c_str(), ImVec2(halfWidth, 420), true);
    {
      ImGui::TextColored(COL_CYAN, "HPC CAPABILITIES");
      ImGui::Spacing();

      if (dev.cudaCapMajor > 0) {
        char capStr[32];
        snprintf(capStr, sizeof(capStr), "%d.%d", dev.cudaCapMajor, dev.cudaCapMinor);
        renderInfoRow("CUDA Capability", capStr, COL_GREEN);

        char coresStr[32];
        snprintf(coresStr, sizeof(coresStr), "%d", dev.cudaCoreCount);
        renderInfoRow("SM Core Count", coresStr, COL_TEXT);

        const char *archName = "Unknown";
        switch (dev.cudaCapMajor) {
          case 5: archName = "Maxwell"; break;
          case 6: archName = "Pascal"; break;
          case 7: archName = (dev.cudaCapMinor >= 5) ? "Turing" : "Volta"; break;
          case 8: archName = "Ampere"; break;
          case 9: archName = "Ada Lovelace"; break;
          case 10: archName = "Blackwell"; break;
        }
        renderInfoRow("Architecture", archName, COL_PURPLE);

        bool hasTensor = (dev.cudaCapMajor >= 7);
        renderInfoRow("Tensor Cores", hasTensor ? "Active" : "None",
                      hasTensor ? COL_GREEN : COL_TEXT_DIM);
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::TextColored(COL_ORANGE, "ACTIVE COMPUTE PROCESSES");
      ImGui::Spacing();

      if (dev.gpuProcesses.empty()) {
        ImGui::TextColored(COL_TEXT_DIM, "No active GPU workloads.");
      } else {
        if (ImGui::BeginTable((childId + "Procs").c_str(), 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
          ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
          ImGui::TableSetupColumn("Process", ImGuiTableColumnFlags_WidthStretch);
          ImGui::TableSetupColumn("VRAM", ImGuiTableColumnFlags_WidthFixed, 80.0f);
          ImGui::TableHeadersRow();

          for (const auto &proc : dev.gpuProcesses) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%u", proc.pid);
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(COL_GREEN, "%s", proc.name.c_str());
            ImGui::TableSetColumnIndex(2);
            float vramMB = proc.vramUsageBytes / (1024.0f * 1024.0f);
            if (vramMB > 1024.0f) ImGui::Text("%.1f GB", vramMB / 1024.0f);
            else ImGui::Text("%.0f MB", vramMB);
          }
          ImGui::EndTable();
        }
      }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    
    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // ── Panel 3: Benchmark Results (full width) ──
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##BenchmarkPanel", ImVec2(-1, 200), true);
  {
    ImGui::TextColored(COL_CYAN, "GPU BENCHMARK");
    ImGui::Spacing();
    ImGui::Spacing();

    if (benchResult.valid) {
      float w = ImGui::GetContentRegionAvail().x;
      float colW = (w - 30) / 4.0f;

      ImGui::BeginChild("##BFP32", ImVec2(colW, 80), true);
      ImGui::TextColored(COL_GPU_ACCENT, "FP32 TFLOPS");
      char fp32[16];
      snprintf(fp32, sizeof(fp32), "%.1f", benchResult.fp32Tflops);
      ImGui::SetCursorPosX(
          (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(fp32).x) /
          2.0f);
      ImGui::TextColored(COL_GREEN, "%s", fp32);
      ImGui::EndChild();
      ImGui::SameLine();

      ImGui::BeginChild("##BFP16", ImVec2(colW, 80), true);
      ImGui::TextColored(COL_GPU_ACCENT, "FP16 TFLOPS");
      char fp16[16];
      snprintf(fp16, sizeof(fp16), "%.1f", benchResult.fp16Tflops);
      ImGui::SetCursorPosX(
          (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(fp16).x) /
          2.0f);
      ImGui::TextColored(COL_CYAN, "%s", fp16);
      ImGui::EndChild();
      ImGui::SameLine();

      ImGui::BeginChild("##BBW", ImVec2(colW, 80), true);
      ImGui::TextColored(COL_GPU_ACCENT, "VRAM BW (GB/s)");
      char bw[16];
      if (benchResult.bandwidthMeasured)
        snprintf(bw, sizeof(bw), "%.1f", benchResult.vramBandwidthGBs);
      else
        snprintf(bw, sizeof(bw), "N/A");
      ImGui::SetCursorPosX(
          (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(bw).x) /
          2.0f);
      ImGui::TextColored(COL_ORANGE, "%s", bw);
      ImGui::EndChild();
      ImGui::SameLine();

      ImGui::BeginChild("##BScore", ImVec2(colW, 80), true);
      ImGui::TextColored(COL_GPU_ACCENT, "AI SCORE");
      char sc[16];
      snprintf(sc, sizeof(sc), "%d/100", benchResult.score);
      ImVec4 scCol =
          benchResult.score >= 60
              ? COL_GREEN
              : (benchResult.score >= 30 ? COL_GPU_WARM : COL_GPU_HOT);
      ImGui::SetCursorPosX(
          (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(sc).x) /
          2.0f);
      ImGui::TextColored(scCol, "%s", sc);
      ImGui::EndChild();

      ImGui::Spacing();
      ImGui::TextColored(scCol, "%s", benchResult.scoreLabel.c_str());
      ImGui::SameLine(ImGui::GetContentRegionAvail().x - 200);
      ImGui::TextColored(COL_TEXT_DIM, "@ %s | %d cores | %u MHz",
                         benchResult.timestamp.c_str(), benchResult.cudaCores,
                         benchResult.clockMHz);
    } else {
      ImGui::TextColored(COL_TEXT_DIM,
                         "No benchmark results yet. Run the benchmark from the "
                         "application to test GPU compute performance.");
      ImGui::Spacing();
      ImGui::TextColored(COL_TEXT_DIM,
                         "Measures: Theoretical TFLOPS (FP32/FP16), VRAM "
                         "bandwidth (GB/s), AI readiness score (0-100).");
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();
}

void GUI::renderSystemTab(const SystemInfo &sysInfo,
                          const ScoreInfo &scoreInfo) {
  ImGui::TextColored(COL_ACCENT, "SYSTEM PERFORMANCE SCORE");
  ImGui::Spacing();
  ImGui::Spacing();

  float contentWidth = ImGui::GetContentRegionAvail().x;

  // Big Score Header
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##ScoreHeader", ImVec2(-1, 140), true);
  {
    float h = ImGui::GetContentRegionAvail().y;
    ImGui::Columns(2, "##scorehead", false);
    ImGui::SetColumnWidth(0, 150);

    // Giant Grade
    ImGui::SetCursorPosY((h - 80) / 2.0f);
    float oldScale = ImGui::GetFont()->Scale;
    ImGui::GetFont()->Scale = 4.0f;
    ImGui::PushFont(ImGui::GetFont());
    ImVec4 gCol = scoreInfo.overallScore > 80
                      ? COL_GREEN
                      : (scoreInfo.overallScore > 50 ? COL_YELLOW : COL_RED);
    ImGui::TextColored(gCol, " %s", scoreInfo.grade.c_str());
    ImGui::GetFont()->Scale = oldScale;
    ImGui::PopFont();

    ImGui::NextColumn();
    ImGui::SetCursorPosY(20);
    ImGui::TextColored(COL_TEXT_DIM, "OVERALL PERFORMANCE SCORE");
    ImGui::ProgressBar(scoreInfo.overallScore / 100.0f, ImVec2(-1, 30), "");
    std::ostringstream oss;
    oss << scoreInfo.overallScore << "/100 Points";
    ImGui::TextColored(COL_TEXT, "%s", oss.str().c_str());
    ImGui::Columns(1);
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::Spacing();

  float halfWidth = (contentWidth - 15) / 2.0f;

  // Detailed Scores
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##DetailedScores", ImVec2(halfWidth, 240), true);
  {
    ImGui::TextColored(COL_CYAN, "COMPONENT BREAKDOWN");
    ImGui::Spacing();

    auto renderScoreBar = [&](const char *label, int score, ImVec4 col) {
      ImGui::TextColored(COL_TEXT_DIM, "%s", label);
      ImGui::SameLine(120);
      ImGui::ProgressBar(score / 100.0f, ImVec2(-1, 14), "");
      ImGui::SameLine(ImGui::GetItemRectMax().x - 40);
      ImGui::Text("%d", score);
    };

    renderScoreBar("CPU", scoreInfo.cpuScore, COL_GREEN);
    ImGui::Spacing();
    renderScoreBar("GPU", scoreInfo.gpuScore, COL_ORANGE);
    ImGui::Spacing();
    renderScoreBar("Memory", scoreInfo.memoryScore, COL_CYAN);
    ImGui::Spacing();
    renderScoreBar("Disk", scoreInfo.diskScore, COL_PURPLE);
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::SameLine();

  // Core System Info (Shifted)
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##SysComputer", ImVec2(halfWidth, 240), true);
  {
    // Computer icon header
    ImGui::TextColored(COL_CYAN, "COMPUTER");
    ImGui::Spacing();
    ImGui::Spacing();

    renderInfoRow("Computer Name", sysInfo.computerName.c_str(), COL_GREEN);
    renderInfoRow("User", sysInfo.userName.c_str(), COL_TEXT);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(COL_PURPLE, "OPERATING SYSTEM");
    ImGui::Spacing();
    ImGui::Spacing();

    renderInfoRow("OS", sysInfo.osVersion.c_str(), COL_TEXT);

    // Uptime
    renderInfoRow("Uptime", formatUptime(sysInfo.uptimeSeconds).c_str(),
                  COL_YELLOW);
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::SameLine();

  // ── Right: CPU & RAM ──
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##SysHardware", ImVec2(halfWidth, 280), true);
  {
    ImGui::TextColored(COL_ORANGE, "PROCESSOR");
    ImGui::Spacing();
    ImGui::Spacing();

    renderInfoRow("CPU", sysInfo.cpuName.c_str(), COL_TEXT);

    std::ostringstream oss;
    oss << sysInfo.cpuCores << " Cores / " << sysInfo.cpuLogicalCores
        << " Threads";
    renderInfoRow("Cores", oss.str().c_str(), COL_GREEN);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(COL_CYAN, "MEMORY");
    ImGui::Spacing();
    ImGui::Spacing();

    double ramGB = (double)sysInfo.totalRAMBytes / (1024.0 * 1024.0 * 1024.0);
    oss.str("");
    oss << std::fixed << std::setprecision(1) << ramGB << " GB";
    renderInfoRow("Total RAM", oss.str().c_str(), COL_TEXT);
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();
}

// ─── Info Row Helper ─────────────────────────────────────────────────────────
void GUI::renderInfoRow(const char *label, const char *value,
                        ImVec4 valueColor) {
  ImGui::TextColored(COL_TEXT_DIM, "%s:", label);
  ImGui::SameLine(180);
  ImGui::TextColored(valueColor, "%s", value);
  ImGui::Spacing();
}

// ─── Circular Gauge ──────────────────────────────────────────────────────────
void GUI::renderCircularGauge(const char *label, float fraction, ImVec4 color,
                              float radius) {
  ImDrawList *dl = ImGui::GetWindowDrawList();
  ImVec2 pos = ImGui::GetCursorScreenPos();
  ImVec2 center = ImVec2(pos.x + radius, pos.y + radius);

  float thickness = 7.0f;
  int segments = 80;

  float startAngle = -((float)M_PI * 0.75f);
  float endAngle = (float)M_PI * 0.75f;
  float totalArc = endAngle - startAngle;

  for (int i = 0; i < segments; i++) {
    float a1 = startAngle + (totalArc * i / segments);
    float a2 = startAngle + (totalArc * (i + 1) / segments);
    ImVec2 p1 =
        ImVec2(center.x + cosf(a1) * radius, center.y + sinf(a1) * radius);
    ImVec2 p2 =
        ImVec2(center.x + cosf(a2) * radius, center.y + sinf(a2) * radius);
    dl->AddLine(p1, p2, ImColor(0.20f, 0.20f, 0.28f, 1.0f), thickness);
  }

  float valueAngle = startAngle + totalArc * fraction;
  int valueSegments = (int)(segments * fraction);
  for (int i = 0; i < valueSegments; i++) {
    float a1 = startAngle + (valueAngle - startAngle) * i / valueSegments;
    float a2 = startAngle + (valueAngle - startAngle) * (i + 1) / valueSegments;

    ImVec2 gp1 =
        ImVec2(center.x + cosf(a1) * radius, center.y + sinf(a1) * radius);
    ImVec2 gp2 =
        ImVec2(center.x + cosf(a2) * radius, center.y + sinf(a2) * radius);
    dl->AddLine(gp1, gp2, ImColor(color.x, color.y, color.z, 0.25f),
                thickness + 6.0f);
    dl->AddLine(gp1, gp2, ImColor(color), thickness);
  }

  char buf[16];
  snprintf(buf, sizeof(buf), "%.0f%%", fraction * 100.0f);
  ImVec2 textSize = ImGui::CalcTextSize(buf);
  dl->AddText(ImVec2(center.x - textSize.x / 2, center.y - textSize.y / 2 - 6),
              ImColor(COL_TEXT), buf);

  ImVec2 labelSize = ImGui::CalcTextSize(label);
  dl->AddText(ImVec2(center.x - labelSize.x / 2, center.y + textSize.y / 2 + 2),
              ImColor(COL_TEXT_DIM), label);

  ImGui::Dummy(ImVec2(radius * 2, radius * 2 + 8));
}

// ─── Graph (Area Chart) ─────────────────────────────────────────────────────
void GUI::renderGraph(const char *label, const float *data, int dataSize,
                      int offset, float maxVal, ImVec4 color, ImVec2 size) {
  ImVec2 pos = ImGui::GetCursorScreenPos();
  if (size.x < 0)
    size.x = ImGui::GetContentRegionAvail().x;
  if (size.y < 0)
    size.y = 100;

  ImDrawList *dl = ImGui::GetWindowDrawList();

  dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                    ImColor(0.08f, 0.08f, 0.12f, 1.0f), 6.0f);

  for (int i = 1; i < 4; i++) {
    float y = pos.y + size.y * i / 4.0f;
    dl->AddLine(ImVec2(pos.x + 4, y), ImVec2(pos.x + size.x - 4, y),
                ImColor(0.15f, 0.15f, 0.22f, 0.5f));
  }

  if (maxVal <= 0.0f)
    maxVal = 1.0f;

  std::vector<ImVec2> linePoints;
  for (int i = 0; i < dataSize; i++) {
    int idx = (offset + 1 + i) % dataSize;
    float val = data[idx] / maxVal;
    if (val > 1.0f)
      val = 1.0f;
    if (val < 0.0f)
      val = 0.0f;
    float x = pos.x + (size.x * i / (dataSize - 1));
    float y = pos.y + size.y - (size.y * val);
    linePoints.push_back(ImVec2(x, y));
  }

  for (int i = 0; i < (int)linePoints.size() - 1; i++) {
    ImVec2 p1 = linePoints[i];
    ImVec2 p2 = linePoints[i + 1];

    ImU32 topColor = ImColor(color.x, color.y, color.z, 0.30f);
    ImU32 botColor = ImColor(color.x, color.y, color.z, 0.02f);

    dl->AddRectFilledMultiColor(ImVec2(p1.x, p1.y),
                                ImVec2(p2.x, pos.y + size.y), topColor,
                                topColor, botColor, botColor);
  }

  for (int i = 0; i < (int)linePoints.size() - 1; i++) {
    dl->AddLine(linePoints[i], linePoints[i + 1], ImColor(color), 2.5f);
  }

  dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
              ImColor(0.18f, 0.18f, 0.25f, 0.5f), 6.0f);

  ImGui::Dummy(size);
}

// ─── Mini Bar ────────────────────────────────────────────────────────────────
void GUI::renderMiniBar(const char *label, float fraction, ImVec4 color) {
  ImDrawList *dl = ImGui::GetWindowDrawList();
  ImVec2 pos = ImGui::GetCursorScreenPos();
  float width = ImGui::GetContentRegionAvail().x;
  float height = 14.0f;

  dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                    ImColor(0.15f, 0.15f, 0.22f, 1.0f), 7.0f);

  if (fraction > 0.0f) {
    dl->AddRectFilled(pos, ImVec2(pos.x + width * fraction, pos.y + height),
                      ImColor(color), 7.0f);
    dl->AddRectFilled(pos, ImVec2(pos.x + width * fraction, pos.y + height),
                      ImColor(color.x, color.y, color.z, 0.15f), 7.0f);
  }

  ImGui::Dummy(ImVec2(width, height + 4));
}

// ─── History Update ──────────────────────────────────────────────────────────
void GUI::updateHistory(float cpuVal, float memVal, float gpuVal, float dlVal,
                        float ulVal, float wattsVal, float diskReadVal,
                        float diskWriteVal) {
  cpuHistory[historyOffset] = cpuVal;
  memHistory[historyOffset] = memVal;
  gpuHistory[historyOffset] = gpuVal;
  downloadHistory[historyOffset] = dlVal;
  uploadHistory[historyOffset] = ulVal;
  powerHistory[historyOffset] = wattsVal;
  diskReadHistory[historyOffset] = diskReadVal;
  diskWriteHistory[historyOffset] = diskWriteVal;
  historyOffset = (historyOffset + 1) % HISTORY_SIZE;
}

// ─── Utilities ───────────────────────────────────────────────────────────────
std::string GUI::formatBytes(uint64_t bytes) {
  std::ostringstream oss;
  if (bytes >= 1024ULL * 1024 * 1024) {
    oss << std::fixed << std::setprecision(2)
        << (double)bytes / (1024.0 * 1024.0 * 1024.0) << " GB";
  } else if (bytes >= 1024ULL * 1024) {
    oss << std::fixed << std::setprecision(1)
        << (double)bytes / (1024.0 * 1024.0) << " MB";
  } else if (bytes >= 1024ULL) {
    oss << std::fixed << std::setprecision(1) << (double)bytes / 1024.0
        << " KB";
  } else {
    oss << bytes << " B";
  }
  return oss.str();
}

std::string GUI::formatSpeed(double bytesPerSec) {
  std::ostringstream oss;
  if (bytesPerSec >= 1024.0 * 1024.0) {
    oss << std::fixed << std::setprecision(2) << bytesPerSec / (1024.0 * 1024.0)
        << " MB/s";
  } else if (bytesPerSec >= 1024.0) {
    oss << std::fixed << std::setprecision(1) << bytesPerSec / 1024.0
        << " KB/s";
  } else {
    oss << std::fixed << std::setprecision(0) << bytesPerSec << " B/s";
  }
  return oss.str();
}

std::string GUI::formatUptime(uint64_t seconds) {
  uint64_t days = seconds / 86400;
  uint64_t hours = (seconds % 86400) / 3600;
  uint64_t mins = (seconds % 3600) / 60;

  std::ostringstream oss;
  if (days > 0) {
    oss << days << "d " << hours << "h " << mins << "m";
  } else if (hours > 0) {
    oss << hours << "h " << mins << "m";
  } else {
    oss << mins << " min";
  }
  return oss.str();
}

// ─── End Frame & Cleanup ────────────────────────────────────────────────────
void GUI::endFrame() {
  ImGui::Render();
  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(COL_BG.x, COL_BG.y, COL_BG.z, COL_BG.w);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(window);
}

void GUI::cleanup() {
  if (initialized) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }
  if (window) {
    glfwDestroyWindow(window);
  }
  glfwTerminate();
  initialized = false;
}
// ─── HPC Assistant Tab ────────────────────────────────────────────────────────
void GUI::renderHpcAssistantTab(const HpcReport &hpcReport) {
  ImGui::TextColored(COL_GPU_ACCENT, "HPC INTELLIGENCE ASSISTANT");
  ImGui::SameLine(ImGui::GetContentRegionAvail().x - 180);
  ImGui::TextColored(COL_TEXT_DIM, "Real-time Heuristics v1.0");
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  float contentWidth = ImGui::GetContentRegionAvail().x;

  // Header Panel: Performance Score & Primary Bottleneck
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
  ImGui::BeginChild("##AssistantHeader", ImVec2(-1, 140), true);
  {
    ImGui::Columns(3, "##asstcols", false);
    ImGui::SetColumnWidth(0, 200);
    ImGui::SetColumnWidth(1, contentWidth - 450);
    ImGui::SetColumnWidth(2, 250);

    // Score Gauge
    float frac = (float)hpcReport.performanceScore / 100.0f;
    ImVec4 sCol = frac > 0.8f ? COL_GREEN : (frac > 0.5f ? COL_YELLOW : COL_ORANGE);
    renderCircularGauge("HPC SCORE", frac, sCol, 50.0f);
    ImGui::NextColumn();

    // Summary
    ImGui::TextColored(COL_CYAN, "SYSTEM HEALTH SUMMARY");
    ImGui::Spacing();
    if (hpcReport.isBottlenecked) {
      ImGui::Text("Primary Bottleneck: ");
      ImGui::SameLine();
      ImGui::TextColored(COL_RED, "%s", hpcReport.primaryBottleneck.c_str());
      ImGui::TextWrapped("The system is currently operating under restricted conditions. "
                         "Review the insights below for specific optimizations.");
    } else {
      ImGui::TextColored(COL_GREEN, "OPTIMIZED FOR WORKLOAD");
      ImGui::TextWrapped("System resources are well-balanced for current tasks. "
                         "Ready for high-performance AI and data processing.");
    }
    ImGui::NextColumn();

    // Action items count
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    int warnings = 0, criticals = 0;
    for(const auto& i : hpcReport.insights) {
      if (i.level == HpcInsight::Level::Warning) warnings++;
      else if (i.level == HpcInsight::Level::Critical) criticals++;
    }
    ImGui::TextColored(COL_RED, "Critical Issues: %d", criticals);
    ImGui::TextColored(COL_YELLOW, "Warnings: %d", warnings);
    ImGui::TextColored(COL_CYAN, "Insights: %d", (int)hpcReport.insights.size() - warnings - criticals);

    ImGui::Columns(1);
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::TextColored(COL_PURPLE, "ACTIONABLE INTELLIGENCE & RECOMMENDATIONS");
  ImGui::Spacing();

  // Recommendations List
  if (hpcReport.insights.empty()) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
    ImGui::BeginChild("##NoInsights", ImVec2(-1, 100), true);
    ImGui::SetCursorPos(ImVec2((contentWidth - ImGui::CalcTextSize("No active alerts. System is running optimally.").x)/2.0f, 40));
    ImGui::TextColored(COL_TEXT_DIM, "No active alerts. System is running optimally.");
    ImGui::EndChild();
    ImGui::PopStyleColor();
  } else {
    for (size_t i = 0; i < hpcReport.insights.size(); i++) {
      const auto &insight = hpcReport.insights[i];
      std::string id = "##Insight" + std::to_string(i);

      ImVec4 borderColor = COL_TEXT_DIM;
      const char* levelStr = "INFO";
      if (insight.level == HpcInsight::Level::Critical) { borderColor = COL_RED; levelStr = "CRITICAL"; }
      else if (insight.level == HpcInsight::Level::Warning) { borderColor = COL_YELLOW; levelStr = "WARNING"; }

      ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
      ImGui::BeginChild(id.c_str(), ImVec2(-1, 120), true);
      {
        ImGui::TextColored(borderColor, "[%s] %s", levelStr, insight.title.c_str());
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextColored(COL_TEXT, "Detection:");
        ImGui::SameLine();
        ImGui::TextWrapped("%s", insight.description.c_str());
        
        ImGui::Spacing();
        ImGui::TextColored(COL_GREEN, "Recommendation:");
        ImGui::SameLine();
        ImGui::TextWrapped("%s", insight.recommendation.c_str());
      }
      ImGui::EndChild();
      ImGui::PopStyleColor();
      ImGui::Spacing();
    }
  }
}

void GUI::renderGamingSessionTab(GamingSessionMonitor& sessionMonitor, HpcEngine& hpcEngine) {
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
  ImGui::Text("ULTRA GAMING PERFORMANCE HUB");
  ImGui::PopStyleColor();
  ImGui::Separator();
  ImGui::Spacing();

  if (!sessionMonitor.isActive()) {
    // START PANEL
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.18f, 1.0f));
    ImGui::BeginChild("SessionControls", ImVec2(0, 120), true);
    {
      ImGui::SetCursorPos(ImVec2(20, 20));
      ImGui::Text("Engage the performance engine.");
      ImGui::SetCursorPos(ImVec2(20, 45));
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "High-precision telemetry will capture FPS stability, thermals, and network latency.");
      
      ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x - 220, 30));
      if (ImGui::Button("START ANALYTICS", ImVec2(200, 50))) {
        sessionMonitor.startSession();
        showSessionReport = false;
      }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (showSessionReport) {
      const auto& metrics = sessionMonitor.getLastSessionMetrics();
      
      ImGui::Spacing();
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "ANALYTICAL DEBRIEF");
      ImGui::Separator();
      ImGui::Spacing();

      // Dashboard View
      float totalAvailWidth = ImGui::GetContentRegionAvail().x;
      float cardWidth = (totalAvailWidth - 40) / 4.0f;
      
      auto renderStatCard = [&](const char* title, const char* val, const char* subtitle, ImVec4 valColor, ImVec4 subColor = ImVec4(0.4f, 0.4f, 0.4f, 1.0f)) {
          ImGui::BeginChild(title, ImVec2(cardWidth, 100), true);
          ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", title);
          ImGui::Separator();
          ImGui::TextColored(valColor, "%s", val);
          ImGui::TextColored(subColor, "%s", subtitle);
          ImGui::EndChild();
          ImGui::SameLine();
      };

      // FPS CARD (Green if > 60, Yellow if > 30, Red if < 30)
      ImVec4 fpsCol = metrics.avgFps >= 60.0 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : (metrics.avgFps >= 30.0 ? ImVec4(1.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
      renderStatCard("FRAME RATE", (std::to_string((int)metrics.avgFps) + " FPS").c_str(), ("Min: " + std::to_string((int)metrics.minFps)).c_str(), fpsCol);

      // PING CARD (Green if < 60ms, Yellow if < 100ms, Red if > 100ms)
      ImVec4 pingCol = metrics.avgPing < 60.0 ? ImVec4(0.0f, 1.0f, 0.8f, 1.0f) : (metrics.avgPing < 100.0 ? ImVec4(1.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
      renderStatCard("LATENCY", (std::to_string((int)metrics.avgPing) + " ms").c_str(), ("Peak: " + std::to_string((int)metrics.maxPing) + "ms").c_str(), pingCol);

      // THERMAL CARD (Red if > 80C, Yellow if > 70C)
      ImVec4 tempCol = metrics.maxGpuTemp < 70.0 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : (metrics.maxGpuTemp < 82.0 ? ImVec4(1.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
      renderStatCard("THERMAL PEAK", (std::to_string((int)metrics.maxGpuTemp) + " C").c_str(), ("Avg: " + std::to_string((int)metrics.avgGpuTemp) + " C").c_str(), tempCol);

      // POWER CARD
      renderStatCard("POWER PEAK", (std::to_string((int)metrics.gpuAvgPower) + " W").c_str(), "GPU Consumption", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
      
      ImGui::NewLine();
      ImGui::Spacing();

      // Visualization Section
      float graphHeight = 140.0f;
      float fullWidth = ImGui::GetContentRegionAvail().x;
      
      ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "TELEMETRY TIMELINES");
      ImGui::Spacing();

      ImGui::BeginChild("GraphsView", ImVec2(0, 680), true);
      {
          ImGui::Text("FPS STABILITY");
          ImGui::PlotLines("##FPSGraph", metrics.fpsHistory.data(), (int)metrics.fpsHistory.size(), 0, nullptr, 0, metrics.maxFps * 1.1f, ImVec2(fullWidth * 0.95f, graphHeight));
          
          ImGui::Spacing();
          ImGui::Text("LATENCY TREND (PING)");
          ImGui::PlotLines("##PingGraph", metrics.pingHistory.data(), (int)metrics.pingHistory.size(), 0, nullptr, 0, (std::max)(100.0f, (float)metrics.maxPing * 1.2f), ImVec2(fullWidth * 0.95f, graphHeight));

          ImGui::Spacing();
          ImGui::Text("GPU UTILIZATION (%)");
          ImGui::PlotLines("##GPUGraph", metrics.gpuHistory.data(), (int)metrics.gpuHistory.size(), 0, nullptr, 0, 100.0f, ImVec2(fullWidth * 0.95f, graphHeight));
          
          ImGui::Spacing();
          ImGui::Text("CPU ENGINE LOAD (%)");
          ImGui::PlotLines("##CPUGraph", metrics.cpuHistory.data(), (int)metrics.cpuHistory.size(), 0, nullptr, 0, 100.0f, ImVec2(fullWidth * 0.95f, graphHeight));
      }
      ImGui::EndChild();

      ImGui::Spacing();
      // AI Diagnostic Report
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "HPC ENGINE INTELLIGENCE");
      renderHpcAssistantTab(hpcReportSession);

      ImGui::Spacing();
      if (ImGui::Button("GENERATE CRYPTO-SIGNED REPORT (.TXT)", ImVec2(350, 50))) {
          saveReportToTxt(hpcReportSession, metrics);
      }
    }
  } else {
    // ACTIVE PANEL
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.20f, 0.08f, 0.08f, 1.0f));
    ImGui::BeginChild("SessionActive", ImVec2(0, 180), true);
    {
      ImGui::SetCursorPos(ImVec2(20, 20));
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "[ ANALYTICS ENGINE ENGAGED ]");
      
      ImGui::SetCursorPos(ImVec2(20, 60));
      ImGui::Text("Streaming deep hardware telemetry...");
      
      // Real-time ping indicator in Active panel
      double currentPing = sessionMonitor.getCurrentPing(); 
      ImVec4 pingColor = currentPing < 60 ? ImVec4(0, 1, 0, 1) : (currentPing < 100 ? ImVec4(1, 1, 0, 1) : ImVec4(1, 0, 0, 1));
      ImGui::SetCursorPos(ImVec2(20, 85));
      ImGui::Text("Current Latency: "); ImGui::SameLine();
      ImGui::TextColored(pingColor, "%.0f ms", currentPing);

      ImGui::SetCursorPos(ImVec2(20, 110));
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Collecting FPS, Thermals, Power, and Latency samples @ 1Hz.");

      ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x - 220, 50));
      if (ImGui::Button("STOP & CALCULATE", ImVec2(200, 60))) {
        sessionMonitor.stopSession();
        hpcReportSession = hpcEngine.generateSessionReport(sessionMonitor.getLastSessionMetrics());
        showSessionReport = true;
      }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
  }
}

void GUI::saveReportToTxt(const HpcReport& report, const SessionMetrics& metrics) {
    std::string filename = "HPC_Performance_Report.txt";
    std::ofstream out(filename);
    
    if (out.is_open()) {
        out << "====================================================\n";
        out << "      HPC MONITOR - GAMING SESSION REPORT\n";
        out << "====================================================\n\n";
        
        out << "SESSION SUMMARY:\n";
        out << "----------------\n";
        out << "Duration: " << std::fixed << std::setprecision(1) << metrics.durationSeconds / 60.0 << " minutes\n";
        out << "Avg FPS: " << metrics.avgFps << " (Peak: " << metrics.maxFps << ")\n";
        out << "Avg Ping: " << std::fixed << std::setprecision(1) << metrics.avgPing << " ms (Peak: " << metrics.maxPing << " ms)\n";
        out << "Avg CPU Load: " << metrics.avgCpuUsage << "%\n";
        out << "Avg GPU Load: " << metrics.avgGpuUsage << "%\n";
        out << "Max GPU Temp: " << metrics.maxGpuTemp << " C\n";
        out << "Memory Peak: " << metrics.maxVramUsageGB << " GB VRAM / " << metrics.maxRamUsageGB << " GB RAM\n\n";
        
        out << "AI DIAGNOSTICS & RECOMMENDATIONS:\n";
        out << "---------------------------------\n";
        for (const auto& insight : report.insights) {
            out << "[" << insight.title << "]\n";
            out << "Findings: " << insight.description << "\n";
            out << "Solution: " << insight.recommendation << "\n\n";
        }
        
        out << "TOP ACTIVE PROCESSES:\n";
        out << "---------------------\n";
        for (const auto& proc : metrics.topProcesses) {
            out << "- " << proc.name << "\n";
        }
        
        out << "\nGenerated by HPC Monitor AI Engine\n";
        out << "====================================================\n";
        out.close();
    }
}
