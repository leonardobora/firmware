#include "PowerManager.h"

#include <esp_sleep.h>

void PowerManager::begin() {
  _mode = PowerMode::WATCH;
  _lastActivityTime = millis();
  disableWiFi();
}

void PowerManager::update() {
  if (_mode != PowerMode::WATCH) return;

  uint32_t elapsed = millis() - _lastActivityTime;

  if (elapsed >= _sleepTimeoutMs) {
    enterSleep();
  } else if (elapsed >= _dimTimeoutMs) {
    turnOffDisplay();
  }
}

void PowerManager::setMode(PowerMode mode) {
  _mode = mode;
  _lastActivityTime = millis();

  switch (mode) {
    case PowerMode::WATCH:
      disableWiFi();
      break;
    case PowerMode::ASSISTANT:
      enableWiFi();
      break;
    case PowerMode::HACKER:
      // WiFi/LoRa enabled per tool — no blanket toggle
      break;
    case PowerMode::SLEEP:
      enterSleep();
      break;
  }
}

void PowerManager::enterSleep(uint64_t sleepTimeUs) {
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 1);  // Button wake
  esp_sleep_enable_ext1_wakeup(
      1ULL << GPIO_NUM_0,
      ESP_EXT1_WAKEUP_ALL_LOW
  );

  if (sleepTimeUs > 0) {
    esp_sleep_enable_timer_wakeup(sleepTimeUs);
  }

  esp_deep_sleep_start();
}

void PowerManager::dimDisplay() {
  // TODO: Integrate with Bruce display API
  // e.g., tft->setBrightness(30);
}

void PowerManager::turnOffDisplay() {
  // TODO: Integrate with Bruce display API
  // e.g., tft->writecommand(TFT_DISPOFF);
  //       tft->writecommand(TFT_SLPIN);
}

void PowerManager::enableWiFi() {
  // TODO: Integrate with Bruce WiFi API
  // e.g., WiFi.begin(ssid, password);
}

void PowerManager::disableWiFi() {
  // TODO: Integrate with Bruce WiFi API
  // e.g., WiFi.disconnect(true);
  //       WiFi.mode(WIFI_OFF);
}
