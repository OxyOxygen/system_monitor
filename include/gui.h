#pragma once

#include "cpu_monitor.h"
#include "disk_monitor.h"
#include "gpu_monitor.h"
#include "memory_monitor.h"
#include "power_monitor.h"

struct GLFWwindow;

class GUI {
public:
  GUI();
  ~GUI();

  bool init(const char *title, int width, int height);
  bool shouldClose();
  void beginFrame();
  void render(double cpuUsage, const MemoryInfo &memInfo,
              const DiskInfo &diskInfo, const GpuInfo &gpuInfo,
              const PowerInfo &powerInfo);
  void endFrame();
  void cleanup();

private:
  GLFWwindow *window;
  bool initialized;

  void renderCpuWidget(double cpuUsage);
  void renderMemoryWidget(const MemoryInfo &info);
  void renderDiskWidget(const DiskInfo &info);
  void renderGpuWidget(const GpuInfo &info);
  void renderPowerWidget(const PowerInfo &info);
  void setupStyle();
};
