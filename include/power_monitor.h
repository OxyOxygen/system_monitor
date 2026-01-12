#pragma once

#include <windows.h>

struct PowerInfo {
  bool onBattery;     // AC mi Battery mi?
  int batteryPercent; // Pil yüzdesi (0-100, -1 if unknown)
  int estimatedTime;  // Tahmini süre (dakika), -1 if unknown
  bool isCharging;    // Şarj oluyor mu?
  bool hasBattery;    // Pil var mı?
  bool batterySaver;  // Battery saver mode (future use)
};

class PowerMonitor {
public:
  PowerMonitor() = default;
  ~PowerMonitor() = default;

  PowerInfo getPowerInfo();
};
