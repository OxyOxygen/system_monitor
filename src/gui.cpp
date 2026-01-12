#include "gui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iomanip>
#include <sstream>

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

GUI::GUI() : window(nullptr), initialized(false) {}

GUI::~GUI() { cleanup(); }

bool GUI::init(const char *title, int width, int height) {
  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return false;

  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // Create window with graphics context
  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (window == nullptr)
    return false;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Setup style
  setupStyle();

  initialized = true;
  return true;
}

void GUI::setupStyle() {
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 *colors = style.Colors;

  // Dark theme with modern colors
  colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.27f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.32f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.11f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.17f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.33f, 0.46f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.23f, 0.32f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.33f, 0.46f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.16f, 0.20f, 0.28f, 1.00f);

  // Rounded corners
  style.WindowRounding = 8.0f;
  style.FrameRounding = 4.0f;
  style.GrabRounding = 4.0f;
  style.ScrollbarRounding = 4.0f;
}

bool GUI::shouldClose() { return glfwWindowShouldClose(window); }

void GUI::beginFrame() {
  glfwPollEvents();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void GUI::render(double cpuUsage, const MemoryInfo &memInfo,
                 const DiskInfo &diskInfo, const GpuInfo &gpuInfo,
                 const PowerInfo &powerInfo) {
  // Main window
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

  ImGui::Begin("System Monitor", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

  // Title
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "SYSTEM MONITOR");
  ImGui::PopFont();
  ImGui::Separator();
  ImGui::Spacing();

  // Render widgets in a 3x2 grid
  float windowWidth = ImGui::GetContentRegionAvail().x;
  float widgetWidth = (windowWidth - 30) / 3; // 3 columns

  // First row: CPU, Memory, Disk
  ImGui::BeginChild("CPU", ImVec2(widgetWidth, 200), true);
  renderCpuWidget(cpuUsage);
  ImGui::EndChild();

  ImGui::SameLine();

  ImGui::BeginChild("Memory", ImVec2(widgetWidth, 200), true);
  renderMemoryWidget(memInfo);
  ImGui::EndChild();

  ImGui::SameLine();

  ImGui::BeginChild("Disk", ImVec2(widgetWidth, 200), true);
  renderDiskWidget(diskInfo);
  ImGui::EndChild();

  ImGui::Spacing();

  // Second row: GPU and Power
  ImGui::BeginChild("GPU", ImVec2(widgetWidth, 200), true);
  renderGpuWidget(gpuInfo);
  ImGui::EndChild();

  ImGui::SameLine();

  ImGui::BeginChild("Power", ImVec2(widgetWidth, 200), true);
  renderPowerWidget(powerInfo);
  ImGui::EndChild();

  ImGui::End();
}

void GUI::renderCpuWidget(double cpuUsage) {
  ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "CPU USAGE");
  ImGui::Spacing();

  // Large percentage display
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1) << cpuUsage << "%%";
  ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", oss.str().c_str());
  ImGui::PopFont();

  ImGui::Spacing();

  // Progress bar
  float fraction = static_cast<float>(cpuUsage) / 100.0f;
  ImVec4 barColor = fraction > 0.8f   ? ImVec4(0.9f, 0.2f, 0.2f, 1.0f)
                    : fraction > 0.5f ? ImVec4(0.9f, 0.7f, 0.2f, 1.0f)
                                      : ImVec4(0.2f, 0.9f, 0.2f, 1.0f);
  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
  ImGui::ProgressBar(fraction, ImVec2(-1, 30));
  ImGui::PopStyleColor();
}

void GUI::renderMemoryWidget(const MemoryInfo &info) {
  ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "MEMORY USAGE");
  ImGui::Spacing();

  // Convert bytes to GB
  double totalGB =
      static_cast<double>(info.totalRAM) / (1024.0 * 1024.0 * 1024.0);
  double usedGB =
      static_cast<double>(info.usedRAM) / (1024.0 * 1024.0 * 1024.0);

  // Display
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1) << usedGB << " / " << totalGB
      << " GB";
  ImGui::Text("%s", oss.str().c_str());

  oss.str("");
  oss << std::fixed << std::setprecision(1) << info.usagePercent << "%%";
  ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", oss.str().c_str());

  ImGui::Spacing();

  // Progress bar
  float fraction = static_cast<float>(info.usagePercent) / 100.0f;
  ImVec4 barColor = fraction > 0.8f   ? ImVec4(0.9f, 0.2f, 0.2f, 1.0f)
                    : fraction > 0.5f ? ImVec4(0.9f, 0.7f, 0.2f, 1.0f)
                                      : ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
  ImGui::ProgressBar(fraction, ImVec2(-1, 30));
  ImGui::PopStyleColor();
}

void GUI::renderDiskWidget(const DiskInfo &info) {
  ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.9f, 1.0f), "DISK USAGE (C:)");
  ImGui::Spacing();

  // Convert bytes to GB
  double totalGB =
      static_cast<double>(info.totalSpace) / (1024.0 * 1024.0 * 1024.0);
  double usedGB =
      static_cast<double>(info.usedSpace) / (1024.0 * 1024.0 * 1024.0);
  double freeGB =
      static_cast<double>(info.freeSpace) / (1024.0 * 1024.0 * 1024.0);

  // Display
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1) << usedGB << " / " << totalGB
      << " GB";
  ImGui::Text("%s", oss.str().c_str());

  oss.str("");
  oss << std::fixed << std::setprecision(1) << freeGB << " GB free";
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", oss.str().c_str());

  ImGui::Spacing();

  // Progress bar
  float fraction = static_cast<float>(info.usagePercent) / 100.0f;
  ImVec4 barColor = fraction > 0.9f   ? ImVec4(0.9f, 0.2f, 0.2f, 1.0f)
                    : fraction > 0.7f ? ImVec4(0.9f, 0.7f, 0.2f, 1.0f)
                                      : ImVec4(0.9f, 0.5f, 0.9f, 1.0f);
  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
  ImGui::ProgressBar(fraction, ImVec2(-1, 30));
  ImGui::PopStyleColor();
}

void GUI::renderGpuWidget(const GpuInfo &info) {
  ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "GPU USAGE");
  ImGui::Spacing();

  if (!info.available) {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                       "GPU monitoring not available");
    ImGui::Text("Requires compatible GPU/drivers");
    return;
  }

  // GPU usage
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1) << info.gpuUsage << "%%";
  ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", oss.str().c_str());

  ImGui::Spacing();

  // Progress bar
  float fraction = static_cast<float>(info.gpuUsage) / 100.0f;
  ImVec4 barColor = fraction > 0.8f   ? ImVec4(0.9f, 0.2f, 0.2f, 1.0f)
                    : fraction > 0.5f ? ImVec4(0.9f, 0.7f, 0.2f, 1.0f)
                                      : ImVec4(0.9f, 0.7f, 0.2f, 1.0f);
  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
  ImGui::ProgressBar(fraction, ImVec2(-1, 30));
  ImGui::PopStyleColor();

  // VRAM info (if available)
  if (info.vramUsed > 0) {
    double vramUsedGB =
        static_cast<double>(info.vramUsed) / (1024.0 * 1024.0 * 1024.0);
    oss.str("");
    oss << "VRAM: " << std::fixed << std::setprecision(1) << vramUsedGB
        << " GB";
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", oss.str().c_str());
  }
}

void GUI::renderPowerWidget(const PowerInfo &info) {
  ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "POWER STATUS");
  ImGui::Spacing();

  if (!info.hasBattery) {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No Battery Detected");
    ImGui::Text("AC Power Only");
    return;
  }

  std::ostringstream oss;

  // Display AC/Battery mode with charging status
  if (info.isCharging) {
    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Charging");
    if (info.batteryPercent >= 0) {
      oss << info.batteryPercent << "%%";
      ImGui::Text("%s", oss.str().c_str());
    }
  } else if (info.onBattery) {
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "On Battery");
    if (info.batteryPercent >= 0) {
      oss << info.batteryPercent << "%%";
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s",
                         oss.str().c_str());
    }
  } else {
    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "AC Power");
    if (info.batteryPercent >= 0) {
      oss << info.batteryPercent << "%%";
      ImGui::Text("%s", oss.str().c_str());
    }
  }

  ImGui::Spacing();

  // Battery progress bar
  if (info.batteryPercent >= 0) {
    float fraction = static_cast<float>(info.batteryPercent) / 100.0f;
    ImVec4 barColor = fraction < 0.2f   ? ImVec4(0.9f, 0.2f, 0.2f, 1.0f)
                      : fraction < 0.5f ? ImVec4(0.9f, 0.7f, 0.2f, 1.0f)
                                        : ImVec4(0.2f, 0.9f, 0.2f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
    ImGui::ProgressBar(fraction, ImVec2(-1, 30));
    ImGui::PopStyleColor();
  }

  // Estimated time remaining
  if (info.onBattery && info.estimatedTime > 0) {
    oss.str("");
    int hours = info.estimatedTime / 60;
    int minutes = info.estimatedTime % 60;
    oss << "~" << hours << "h " << minutes << "m remaining";
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", oss.str().c_str());
  }
}

void GUI::endFrame() {
  ImGui::Render();
  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(0.10f, 0.10f, 0.13f, 1.00f);
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
