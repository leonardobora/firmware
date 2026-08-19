#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>

enum class PowerMode : uint8_t {
  WATCH,      // Display on, BLE on, WiFi off — default, days of battery
  ASSISTANT,  // Display on, BLE on, WiFi on, Mic on — minutes
  HACKER,     // Display on, BLE on, WiFi/LoRa per tool — minutes
  SLEEP       // Deep sleep — weeks
};

class PowerManager {
public:
  void begin();
  void update();
  void setMode(PowerMode mode);
  PowerMode currentMode() const { return _mode; }
  void enterSleep(uint64_t sleepTimeUs = 0);

private:
  PowerMode _mode = PowerMode::WATCH;
  uint32_t _lastActivityTime = 0;
  uint32_t _dimTimeoutMs = 10000;    // 10 seconds
  uint32_t _sleepTimeoutMs = 30000;  // 30 seconds

  void dimDisplay();
  void turnOffDisplay();
  void enableWiFi();
  void disableWiFi();
};

#endif // POWER_MANAGER_H
