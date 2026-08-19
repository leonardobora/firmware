#ifndef GESTURE_ENGINE_H
#define GESTURE_ENGINE_H

#include <Arduino.h>
#include <functional>

enum class InputSource : uint8_t {
  ACCEL,
  BUTTON,
  TOUCH,
  VOICE
};

enum class ActionType : uint8_t {
  SCREEN_WAKE,
  OPEN_ASSISTANT,
  OPEN_HACKER,
  MUSIC_TOGGLE,
  OPEN_LAUNCHER,
  DEEP_SLEEP,
  NONE
};

struct GestureEvent {
  InputSource source;
  ActionType action;
  const char* triggerName;
};

using GestureCallback = std::function<void(const GestureEvent&)>;

class GestureEngine {
public:
  void begin();
  void update();
  void onAction(GestureCallback cb);

  ActionType getPendingAction();

private:
  void detectShake();
  void detectButton();
  void emit(InputSource src, ActionType action, const char* name);

  GestureCallback _callback = nullptr;
  ActionType _pendingAction = ActionType::NONE;

  // Shake detection state
  int16_t _lastAccelX = 0;
  int16_t _lastAccelY = 0;
  int16_t _lastAccelZ = 0;
  uint8_t _shakeCount = 0;
  int8_t _shakeDirection = 0; // -1 = negative, +1 = positive
  uint32_t _shakeWindowStart = 0;
  static constexpr int16_t SHAKE_THRESHOLD = 15000;
  static constexpr uint32_t SHAKE_WINDOW_MS = 500;
  static constexpr uint8_t SHAKE_COUNT_TARGET = 2;

  // Button detection state
  bool _buttonPressed = false;
  uint32_t _buttonPressStart = 0;
  uint32_t _lastButtonRelease = 0;
  uint8_t _buttonClickCount = 0;
  uint32_t _clickWindowStart = 0;
  static constexpr uint32_t DOUBLE_CLICK_WINDOW_MS = 300;
  static constexpr uint32_t HOLD_DURATION_MS = 3000;
};

#endif // GESTURE_ENGINE_H
