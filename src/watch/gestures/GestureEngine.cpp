#include "GestureEngine.h"

void GestureEngine::begin() {
  _lastAccelX = 0;
  _lastAccelY = 0;
  _lastAccelZ = 0;
  _shakeCount = 0;
  _shakeDirection = 0;
  _shakeWindowStart = 0;
  _buttonPressed = false;
  _buttonPressStart = 0;
  _lastButtonRelease = 0;
  _buttonClickCount = 0;
  _clickWindowStart = 0;
  _pendingAction = ActionType::NONE;
}

void GestureEngine::update() {
  detectShake();
  detectButton();
}

void GestureEngine::onAction(GestureCallback cb) {
  _callback = cb;
}

ActionType GestureEngine::getPendingAction() {
  ActionType action = _pendingAction;
  _pendingAction = ActionType::NONE;
  return action;
}

void GestureEngine::detectShake() {
  // --- PLACEHOLDER: read accelerometer ---
  // When Bruce API is available, replace with:
  //   int16_t ax = IMU.ax;
  //   int16_t ay = IMU.ay;
  //   int16_t az = IMU.az;
  int16_t ax = 0;
  int16_t ay = 0;
  int16_t az = 0;

  int16_t deltaX = ax - _lastAccelX;
  _lastAccelX = ax;
  _lastAccelY = ay;
  _lastAccelZ = az;

  if (abs(deltaX) < SHAKE_THRESHOLD) {
    return;
  }

  uint32_t now = millis();

  int8_t currentDir = (deltaX > 0) ? 1 : -1;

  if (_shakeCount == 0 || (now - _shakeWindowStart) > SHAKE_WINDOW_MS) {
    _shakeCount = 1;
    _shakeDirection = currentDir;
    _shakeWindowStart = now;
    return;
  }

  if (currentDir != _shakeDirection) {
    _shakeCount++;
    _shakeDirection = currentDir;

    if (_shakeCount >= SHAKE_COUNT_TARGET * 2) {
      if (deltaX < 0) {
        emit(InputSource::ACCEL, ActionType::OPEN_ASSISTANT, "shake_left_2x");
      } else {
        emit(InputSource::ACCEL, ActionType::OPEN_HACKER, "shake_right_2x");
      }
      _shakeCount = 0;
    }
  }
}

void GestureEngine::detectButton() {
  // --- PLACEHOLDER: read button state ---
  // When Bruce API is available, replace with:
  //   bool pressed = (digitalRead(BTN_PIN) == LOW);
  bool pressed = false;

  uint32_t now = millis();

  if (pressed && !_buttonPressed) {
    _buttonPressed = true;
    _buttonPressStart = now;
  } else if (!pressed && _buttonPressed) {
    _buttonPressed = false;
    uint32_t held = now - _buttonPressStart;

    if (held >= HOLD_DURATION_MS) {
      emit(InputSource::BUTTON, ActionType::DEEP_SLEEP, "hold_power_3s");
      _buttonClickCount = 0;
      return;
    }

    if ((now - _clickWindowStart) > DOUBLE_CLICK_WINDOW_MS) {
      _buttonClickCount = 1;
      _clickWindowStart = now;
    } else {
      _buttonClickCount++;
    }

    _lastButtonRelease = now;
  }

  if (!_buttonPressed && _buttonClickCount == 1 &&
      (now - _lastButtonRelease) > DOUBLE_CLICK_WINDOW_MS) {
    _buttonClickCount = 0;
  }

  if (_buttonClickCount >= 2) {
    emit(InputSource::BUTTON, ActionType::MUSIC_TOGGLE, "double_click_power");
    _buttonClickCount = 0;
  }
}

void GestureEngine::emit(InputSource src, ActionType action, const char* name) {
  GestureEvent event;
  event.source = src;
  event.action = action;
  event.triggerName = name;

  _pendingAction = action;

  if (_callback) {
    _callback(event);
  }
}
