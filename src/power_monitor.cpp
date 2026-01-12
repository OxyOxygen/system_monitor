#include "power_monitor.h"

PowerInfo PowerMonitor::getPowerInfo() {
  PowerInfo info = {};

  SYSTEM_POWER_STATUS powerStatus;
  if (!GetSystemPowerStatus(&powerStatus)) {
    // Failed to get power status
    info.batteryPercent = -1;
    info.estimatedTime = -1;
    info.hasBattery = false;
    return info;
  }

  // Check if battery exists
  info.hasBattery = (powerStatus.BatteryFlag != 128); // 128 = No system battery

  // AC or Battery?
  info.onBattery = (powerStatus.ACLineStatus == 0); // 0 = Offline (on battery)

  // Battery percentage
  if (powerStatus.BatteryLifePercent != 255) {
    info.batteryPercent = powerStatus.BatteryLifePercent;
  } else {
    info.batteryPercent = -1; // Unknown
  }

  // Charging status
  // BatteryFlag: 8 = Charging
  info.isCharging = (powerStatus.BatteryFlag & 8) != 0;

  // Estimated time remaining (in seconds, convert to minutes)
  if (powerStatus.BatteryLifeTime != 0xFFFFFFFF) {
    info.estimatedTime =
        powerStatus.BatteryLifeTime / 60; // Convert seconds to minutes
  } else {
    info.estimatedTime = -1; // Unknown or on AC
  }

  // Battery saver mode (not directly available via this API, set to false for
  // now)
  info.batterySaver = false;

  return info;
}
