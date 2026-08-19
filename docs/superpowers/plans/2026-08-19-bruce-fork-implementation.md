# b0r4-watch Bruce Fork — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fork Bruce firmware and add smartwatch features (watchface, voice assistant, music, notifications, gestures) to create a hybrid hacker tool + personal assistant on the LILYGO T-Watch S3.

**Architecture:** Fork BruceDevices/firmware, add `src/watch/` module tree for watchface, assistant, music, gestures, ANCS, GPS, and energy management. Python server stays independent with new endpoints.

**Tech Stack:** Arduino + PlatformIO, LVGL 8.x, ESP32-S3, RadioLib, IRremoteESP8266, Python FastAPI, WebSocket, BLE GATT.

## Global Constraints

- Hardware: LILYGO T-Watch S3 (ESP32-S3 N16R8, SX1262 915MHz, 16MB flash, 8MB PSRAM)
- Framework: Arduino (via PlatformIO, espressif32@6.3.0)
- UI: LVGL 8.4.0
- License: AGPL (firmware fork), MIT (server)
- Server: Python 3.12+, FastAPI, WebSocket
- Build: `pio run` for firmware, `uvicorn` for server
- All new firmware code goes in `src/watch/` — never modify Bruce core code directly

---

## File Structure

```
b0r4-watch-bruce/                     # New repo (fork of BruceDevices/firmware)
├── src/
│   ├── watch/                         # ALL NEW CODE HERE
│   │   ├── watchface/
│   │   │   ├── WatchFace.h            # Clock face UI
│   │   │   ├── WatchFace.cpp
│   │   │   ├── StatusIcons.h          # Battery, WiFi, BLE, LoRa icons
│   │   │   └── StatusIcons.cpp
│   │   ├── avatar/
│   │   │   ├── Avatar.h               # Animated avatar (GIF/sprite)
│   │   │   ├── Avatar.cpp
│   │   │   └── sprites/               # Animation frames (GIF or PNG sequence)
│   │   ├── assistant/
│   │   │   ├── AssistantClient.h      # WebSocket client for voice pipeline
│   │   │   ├── AssistantClient.cpp
│   │   │   ├── AssistantUI.h          # Conversation view UI
│   │   │   └── AssistantUI.cpp
│   │   ├── music/
│   │   │   ├── MusicPlayer.h          # Music control via WebSocket
│   │   │   ├── MusicPlayer.cpp
│   │   │   ├── MusicUI.h             # Player UI
│   │   │   └── MusicUI.cpp
│   │   ├── gestures/
│   │   │   ├── GestureEngine.h        # Trigger → action mapper
│   │   │   ├── GestureEngine.cpp
│   │   │   └── gestures.json          # Default gesture config
│   │   ├── ancs/
│   │   │   ├── ANCSDisplay.h          # Notification list UI
│   │   │   └── ANCSDisplay.cpp
│   │   ├── gps_ble/
│   │   │   ├── GPSClient.h            # BLE GPS from iPhone
│   │   │   └── GPSClient.cpp
│   │   ├── energy/
│   │   │   ├── PowerManager.h         # Sleep/wake policy
│   │   │   └── PowerManager.cpp
│   │   └── ScreenManager.h            # Navigation between screens
│   │   └── ScreenManager.cpp
│   └── main.cpp                       # Modified: init watch modules
├── server/                            # Copied from b0r4-watch/server/
│   ├── main.py                        # Add /ws/music, /api/notifications, /api/gps
│   └── ...
└── docs/                              # Copied ADRs + glossary
```

---

## Phase 1: Fork & Setup

### Task 1: Fork Bruce and set up repo

**Files:**
- Create: Fork `BruceDevices/firmware` → `leonardobora/b0r4-watch-bruce`
- Create: `docs/` directory with copied ADRs and glossary

- [ ] **Step 1: Fork Bruce on GitHub**

Go to https://github.com/BruceDevices/firmware and click Fork to `leonardobora/b0r4-watch-bruce`.

- [ ] **Step 2: Clone the fork locally**

```bash
git clone https://github.com/leonardobora/b0r4-watch-bruce.git
cd b0r4-watch-bruce
```

- [ ] **Step 3: Verify Bruce builds for T-Watch S3**

```bash
pio run -e twatch-s3
```

Expected: Build succeeds. Note the binary size.

- [ ] **Step 4: Copy docs from original b0r4-watch**

```bash
mkdir -p docs/adr docs/superpowers
cp ../b0r4-watch/docs/adr/*.md docs/adr/
cp ../b0r4-watch/docs/CONTEXT.md docs/
cp ../b0r4-watch/docs/input-system.md docs/
cp ../b0r4-watch/docs/superpowers/specs/*.md docs/superpowers/specs/
```

- [ ] **Step 5: Copy server directory**

```bash
cp -r ../b0r4-watch/server .
```

- [ ] **Step 6: Create src/watch/ directory structure**

```bash
mkdir -p src/watch/{watchface,avatar/sprites,assistant,music,gestures,ancs,gps_ble,energy}
```

- [ ] **Step 7: Commit**

```bash
git add docs/ server/ src/watch/
git commit -m "chore: setup b0r4-watch overlay (docs, server, watch module tree)"
```

---

### Task 2: Understand Bruce's module system

**Files:**
- Read: `src/core/` — display, input, BLE, WiFi setup
- Read: `src/modules/` — CC1101, IR, GPS, NFC, LoRa drivers
- Read: `src/apps/` — how Bruce apps are structured
- Read: `platformio.ini` — build configuration

- [ ] **Step 1: Read Bruce's main entry point**

Read `src/main.cpp` (or equivalent). Understand how Bruce initializes display, input, and launches apps.

- [ ] **Step 2: Read one Bruce app to understand the pattern**

Read a simple app (e.g., `src/apps/` any simple module). Note:
- How apps register with the launcher
- How they use the display (LVGL or direct TFT)
- How they handle input events
- How they're included in the build

- [ ] **Step 3: Read Bruce's display/TFT setup**

Find how Bruce initializes the display driver for T-Watch S3. Note the TFT library used (likely TFT_eSPI or custom), LVGL integration, and screen dimensions.

- [ ] **Step 4: Read Bruce's BLE setup**

Find how Bruce initializes BLE. Note which BLE library is used and how apps access BLE.

- [ ] **Step 5: Document findings**

Write notes in `docs/bruce-integration-notes.md`:
- Display init sequence
- App registration pattern
- BLE initialization
- Input handling pattern

- [ ] **Step 6: Commit**

```bash
git add docs/bruce-integration-notes.md
git commit -m "docs: Bruce integration notes for b0r4-watch modules"
```

---

## Phase 2: Watchface

### Task 3: Minimal watchface (clock + status)

**Files:**
- Create: `src/watch/watchface/WatchFace.h`
- Create: `src/watch/watchface/WatchFace.cpp`
- Create: `src/watch/watchface/StatusIcons.h`
- Create: `src/watch/watchface/StatusIcons.cpp`

**Interfaces:**
- Consumes: Bruce display driver (TFT or LVGL display object)
- Produces: `WatchFace::begin(lv_obj_t* parent)`, `WatchFace::update()`

- [ ] **Step 1: Create WatchFace.h**

```cpp
#pragma once
#include <lvgl.h>

class WatchFace {
public:
    void begin(lv_obj_t* parent);
    void update();  // Call every second

private:
    lv_obj_t* _container = nullptr;
    lv_obj_t* _timeLabel = nullptr;
    lv_obj_t* _dateLabel = nullptr;
    lv_obj_t* _batteryArc = nullptr;
    lv_obj_t* _batteryLabel = nullptr;

    void createTimeDisplay();
    void createDateDisplay();
    void createBatteryIndicator();
};
```

- [ ] **Step 2: Create WatchFace.cpp**

```cpp
#include "WatchFace.h"
#include <time.h>
#include <esp_battery.h>  // Bruce's battery reading

void WatchFace::begin(lv_obj_t* parent) {
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(_container, lv_color_hex(0x000000), 0);

    createTimeDisplay();
    createDateDisplay();
    createBatteryIndicator();
}

void WatchFace::createTimeDisplay() {
    _timeLabel = lv_label_create(_container);
    lv_label_set_text(_timeLabel, "00:00");
    lv_obj_set_style_text_font(_timeLabel, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(_timeLabel, lv_color_hex(0xFFFFFF), 0);
}

void WatchFace::createDateDisplay() {
    _dateLabel = lv_label_create(_container);
    lv_label_set_text(_dateLabel, "Seg, 19 Ago");
    lv_obj_set_style_text_font(_dateLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(_dateLabel, lv_color_hex(0x888888), 0);
}

void WatchFace::createBatteryIndicator() {
    _batteryArc = lv_arc_create(_container);
    lv_arc_set_range(_batteryArc, 0, 100);
    lv_arc_set_value(_batteryArc, 75);
    lv_obj_set_size(_batteryArc, 40, 40);

    _batteryLabel = lv_label_create(_batteryArc);
    lv_label_set_text(_batteryLabel, "75%");
    lv_obj_center(_batteryLabel);
}

void WatchFace::update() {
    time_t now;
    struct tm* timeinfo;
    time(&now);
    timeinfo = localtime(&now);

    char timeBuf[6];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M", timeinfo);
    lv_label_set_text(_timeLabel, timeBuf);

    char dateBuf[32];
    strftime(dateBuf, sizeof(dateBuf), "%a, %d %b", timeinfo);
    lv_label_set_text(_dateLabel, dateBuf);

    // Battery (placeholder — use Bruce's battery API when available)
    int pct = 75;  // TODO: read from hardware
    lv_arc_set_value(_batteryArc, pct);
    char batBuf[8];
    snprintf(batBuf, sizeof(batBuf), "%d%%", pct);
    lv_label_set_text(_batteryLabel, batBuf);
}
```

- [ ] **Step 3: Create StatusIcons.h**

```cpp
#pragma once
#include <lvgl.h>

class StatusIcons {
public:
    void begin(lv_obj_t* parent);
    void update(bool wifi, bool ble, bool lora);

private:
    lv_obj_t* _wifiIcon = nullptr;
    lv_obj_t* _bleIcon = nullptr;
    lv_obj_t* _loraIcon = nullptr;
};
```

- [ ] **Step 4: Create StatusIcons.cpp**

```cpp
#include "StatusIcons.h"

void StatusIcons::begin(lv_obj_t* parent) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 30);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    _wifiIcon = lv_label_create(row);
    lv_label_set_text(_wifiIcon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(_wifiIcon, lv_color_hex(0x888888), 0);

    _bleIcon = lv_label_create(row);
    lv_label_set_text(_bleIcon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(_bleIcon, lv_color_hex(0x888888), 0);

    _loraIcon = lv_label_create(row);
    lv_label_set_text(_loraIcon, LV_SYMBOL_LOOP);  // placeholder for LoRa
    lv_obj_set_style_text_color(_loraIcon, lv_color_hex(0x888888), 0);
}

void StatusIcons::update(bool wifi, bool ble, bool lora) {
    lv_obj_set_style_text_color(_wifiIcon, wifi ? lv_color_hex(0x00FF00) : lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_color(_bleIcon, ble ? lv_color_hex(0x00FF00) : lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_color(_loraIcon, lora ? lv_color_hex(0x00FF00) : lv_color_hex(0x444444), 0);
}
```

- [ ] **Step 5: Test on simulator (if possible)**

```bash
pio run -e simulator
```

If simulator env exists in Bruce, test watchface renders. If not, skip — test on hardware when it arrives.

- [ ] **Step 6: Commit**

```bash
git add src/watch/watchface/
git commit -m "feat(watchface): clock display with time, date, battery, status icons"
```

---

### Task 4: Animated avatar

**Files:**
- Create: `src/watch/avatar/Avatar.h`
- Create: `src/watch/avatar/Avatar.cpp`
- Create: `src/watch/avatar/sprites/idle.png` (placeholder)

**Interfaces:**
- Consumes: LVGL parent object
- Produces: `Avatar::begin(lv_obj_t* parent)`, `Avatar::setState(AvatarState)`

- [ ] **Step 1: Create Avatar.h**

```cpp
#pragma once
#include <lvgl.h>

enum class AvatarState {
    IDLE,
    LISTENING,
    THINKING,
    SPEAKING,
    SURPRISED,
    SLEEPY
};

class Avatar {
public:
    void begin(lv_obj_t* parent);
    void setState(AvatarState state);
    void update();  // Animation tick

private:
    lv_obj_t* _img = nullptr;
    AvatarState _state = AvatarState::IDLE;
    uint32_t _lastFrameTime = 0;
    int _currentFrame = 0;

    // GIF/sprite animation data (to be filled with actual assets)
    struct Animation {
        const void** frames;
        int frameCount;
        uint32_t frameDelayMs;
    };

    // Placeholder: simple LVGL label with emoji
    void showPlaceholder();
};
```

- [ ] **Step 2: Create Avatar.cpp**

```cpp
#include "Avatar.h"

// Placeholder implementation using LVGL label
// Replace with GIF decoder or sprite animation later

void Avatar::begin(lv_obj_t* parent) {
    _img = lv_label_create(parent);
    lv_obj_set_style_text_font(_img, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(_img, lv_color_hex(0xFFFFFF), 0);
    showPlaceholder();
}

void Avatar::setState(AvatarState state) {
    _state = state;
    showPlaceholder();
}

void Avatar::showPlaceholder() {
    const char* emoji;
    switch (_state) {
        case AvatarState::IDLE:      emoji = "😊"; break;
        case AvatarState::LISTENING:  emoji = "👂"; break;
        case AvatarState::THINKING:   emoji = "🤔"; break;
        case AvatarState::SPEAKING:   emoji = "💬"; break;
        case AvatarState::SURPRISED:  emoji = "😲"; break;
        case AvatarState::SLEEPY:     emoji = "😴"; break;
    }
    lv_label_set_text(_img, emoji);
}

void Avatar::update() {
    // Future: animation frame update here
}
```

- [ ] **Step 3: Integrate into WatchFace**

Modify `src/watch/watchface/WatchFace.h` to include Avatar:

```cpp
#include "../avatar/Avatar.h"

// Add to WatchFace class:
private:
    Avatar _avatar;
```

Modify `WatchFace::begin()` to call `_avatar.begin(_container)` after creating the time display.

- [ ] **Step 4: Commit**

```bash
git add src/watch/avatar/ src/watch/watchface/
git commit -m "feat(avatar): placeholder avatar with state system, integrated into watchface"
```

---

### Task 5: Screen manager (swipe navigation)

**Files:**
- Create: `src/watch/ScreenManager.h`
- Create: `src/watch/ScreenManager.cpp`

**Interfaces:**
- Consumes: LVGL display, gesture events
- Produces: `ScreenManager::begin()`, `ScreenManager::switchTo(Screen)`, `ScreenManager::update()`

- [ ] **Step 1: Create ScreenManager.h**

```cpp
#pragma once
#include <lvgl.h>

enum class Screen {
    ASSISTANT,
    WATCHFACE,
    HACKER,
    MUSIC,       // accessed via swipe up from watchface
    NOTIFICATIONS // accessed via swipe down from watchface
};

class ScreenManager {
public:
    void begin(lv_obj_t* display);
    void switchTo(Screen screen);
    Screen current() const { return _current; }
    void update();  // Handle navigation events

private:
    lv_obj_t* _display = nullptr;
    lv_obj_t* _screens[5] = {};  // LVGL objects for each screen
    Screen _current = Screen::WATCHFACE;

    void createScreen(int index, lv_color_t bgColor);
};
```

- [ ] **Step 2: Create ScreenManager.cpp**

```cpp
#include "ScreenManager.h"

void ScreenManager::begin(lv_obj_t* display) {
    _display = display;

    // Create 5 screens as children of display
    createScreen(0, lv_color_hex(0x1A1A2E));  // Assistant - dark blue
    createScreen(1, lv_color_hex(0x000000));  // Watchface - black
    createScreen(2, lv_color_hex(0x0D1117));  // Hacker - dark gray
    createScreen(3, lv_color_hex(0x16213E));  // Music - navy
    createScreen(4, lv_color_hex(0x1A1A2E));  // Notifications - dark blue

    switchTo(Screen::WATCHFACE);
}

void ScreenManager::createScreen(int index, lv_color_t bgColor) {
    _screens[index] = lv_obj_create(_display);
    lv_obj_set_size(_screens[index], LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_screens[index], bgColor, 0);
    lv_obj_set_style_border_width(_screens[index], 0, 0);
    lv_obj_set_style_pad_all(_screens[index], 0, 0);
    lv_obj_add_flag(_screens[index], LV_OBJ_FLAG_HIDDEN);
}

void ScreenManager::switchTo(Screen screen) {
    // Hide current
    if (_screens[(int)_current]) {
        lv_obj_add_flag(_screens[(int)_current], LV_OBJ_FLAG_HIDDEN);
    }

    _current = screen;

    // Show new
    if (_screens[(int)_current]) {
        lv_obj_clear_flag(_screens[(int)_current], LV_OBJ_FLAG_HIDDEN);
    }
}

void ScreenManager::update() {
    // Future: handle swipe gestures to switch screens
    // For now, just stay on current screen
}
```

- [ ] **Step 3: Integrate into main.cpp**

Add to the Bruce main entry point (after Bruce display init):

```cpp
#include "watch/ScreenManager.h"
#include "watch/watchface/WatchFace.h"

static ScreenManager screenMgr;
static WatchFace watchFace;

void setup() {
    // ... Bruce init code ...

    screenMgr.begin(lv_scr_act());
    watchFace.begin(screenMgr.screens[(int)Screen::WATCHFACE]);
}
```

Note: `screens[]` may need to be public or accessed via a getter. Adjust based on Bruce's actual structure.

- [ ] **Step 4: Commit**

```bash
git add src/watch/ScreenManager.h src/watch/ScreenManager.cpp
git commit -m "feat(screens): screen manager with 5-screen navigation skeleton"
```

---

## Phase 3: Core Features

### Task 6: Gesture engine

**Files:**
- Create: `src/watch/gestures/GestureEngine.h`
- Create: `src/watch/gestures/GestureEngine.cpp`
- Create: `src/watch/gestures/gestures.json`

**Interfaces:**
- Consumes: BMA423 accelerometer data (Bruce driver), button events
- Produces: `GestureEngine::begin()`, `GestureEngine::update()`, `GestureEngine::onAction(callback)`

- [ ] **Step 1: Create gestures.json**

```json
{
  "triggers": [
    {
      "name": "wrist_lift",
      "source": "ACCEL",
      "pattern": "wrist_tilt",
      "action": "screen_wake",
      "contexts": ["any"]
    },
    {
      "name": "shake_left_2x",
      "source": "ACCEL",
      "pattern": "shake_x:2:negative",
      "action": "open_assistant",
      "contexts": ["watchface"]
    },
    {
      "name": "shake_right_2x",
      "source": "ACCEL",
      "pattern": "shake_x:2:positive",
      "action": "open_hacker",
      "contexts": ["watchface"]
    },
    {
      "name": "double_click_power",
      "source": "BUTTON",
      "pattern": "double_click:POWER",
      "action": "music_toggle",
      "contexts": ["any"]
    },
    {
      "name": "tap_avatar",
      "source": "TOUCH",
      "pattern": "tap:avatar",
      "action": "open_launcher",
      "contexts": ["watchface"]
    },
    {
      "name": "hold_power_3s",
      "source": "BUTTON",
      "pattern": "hold:POWER:3000",
      "action": "deep_sleep",
      "contexts": ["any"]
    }
  ]
}
```

- [ ] **Step 2: Create GestureEngine.h**

```cpp
#pragma once
#include <functional>

enum class InputSource { ACCEL, BUTTON, TOUCH, VOICE };
enum class ActionType {
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
    void update();  // Poll sensors and detect gestures
    void onAction(GestureCallback cb) { _callback = cb; }

private:
    GestureCallback _callback = nullptr;

    // Accelerometer state
    int16_t _lastAccelX = 0;
    uint32_t _shakeCount = 0;
    uint32_t _shakeStartTime = 0;
    bool _shakeDir = false;  // false=negative, true=positive

    // Button state
    uint32_t _buttonDownTime = 0;
    uint32_t _lastButtonUpTime = 0;
    int _clickCount = 0;

    void detectShake(int16_t accelX);
    void detectButton(bool pressed);
    void emit(ActionType action, const char* name);
};
```

- [ ] **Step 3: Create GestureEngine.cpp**

```cpp
#include "GestureEngine.h"
#include <Arduino.h>

// Thresholds
static const int16_t SHAKE_THRESHOLD = 15000;
static const uint32_t SHAKE_WINDOW_MS = 500;
static const uint32_t DOUBLE_CLICK_MS = 300;
static const uint32_t HOLD_MS = 3000;

void GestureEngine::begin() {
    _shakeCount = 0;
    _clickCount = 0;
}

void GestureEngine::update() {
    // Read accelerometer (placeholder — use Bruce's BMA423 API)
    // int16_t accelX = bma423.getAccelX();
    // detectShake(accelX);

    // Read button (placeholder — use Bruce's button API)
    // bool pressed = button.isPressed();
    // detectButton(pressed);
}

void GestureEngine::detectShake(int16_t accelX) {
    uint32_t now = millis();

    if (abs(accelX) > SHAKE_THRESHOLD) {
        bool currentDir = accelX > 0;

        if (_shakeCount == 0 || (now - _shakeStartTime > SHAKE_WINDOW_MS)) {
            _shakeCount = 1;
            _shakeStartTime = now;
            _shakeDir = currentDir;
        } else if (currentDir == _shakeDir) {
            _shakeCount++;
            if (_shakeCount >= 2) {
                if (_shakeDir) {
                    emit(ActionType::OPEN_HACKER, "shake_right_2x");
                } else {
                    emit(ActionType::OPEN_ASSISTANT, "shake_left_2x");
                }
                _shakeCount = 0;
            }
        }
    }

    if (_shakeCount > 0 && (now - _shakeStartTime > SHAKE_WINDOW_MS)) {
        _shakeCount = 0;
    }
}

void GestureEngine::detectButton(bool pressed) {
    uint32_t now = millis();

    if (pressed && _buttonDownTime == 0) {
        _buttonDownTime = now;
    } else if (!pressed && _buttonDownTime > 0) {
        uint32_t duration = now - _buttonDownTime;
        _buttonDownTime = 0;

        if (duration >= HOLD_MS) {
            emit(ActionType::DEEP_SLEEP, "hold_power_3s");
            _clickCount = 0;
        } else {
            if (_clickCount > 0 && (now - _lastButtonUpTime < DOUBLE_CLICK_MS)) {
                _clickCount++;
            } else {
                _clickCount = 1;
            }
            _lastButtonUpTime = now;

            // Wait for potential second click
            delay(DOUBLE_CLICK_MS);
            if (_clickCount == 2) {
                emit(ActionType::MUSIC_TOGGLE, "double_click_power");
                _clickCount = 0;
            }
        }
    }
}

void GestureEngine::emit(ActionType action, const char* name) {
    if (_callback) {
        GestureEvent event;
        event.source = InputSource::ACCEL;
        event.action = action;
        event.triggerName = name;
        _callback(event);
    }
}
```

- [ ] **Step 4: Commit**

```bash
git add src/watch/gestures/
git commit -m "feat(gestures): gesture engine with shake detection and button combos"
```

---

### Task 7: Assistant WebSocket client

**Files:**
- Create: `src/watch/assistant/AssistantClient.h`
- Create: `src/watch/assistant/AssistantClient.cpp`
- Create: `src/watch/assistant/AssistantUI.h`
- Create: `src/watch/assistant/AssistantUI.cpp`

**Interfaces:**
- Consumes: WiFi connection (Bruce), microphone input (Bruce)
- Produces: `AssistantClient::connect(url)`, `AssistantClient::startListening()`, `AssistantClient::stopListening()`

- [ ] **Step 1: Create AssistantClient.h**

```cpp
#pragma once
#include <Arduino.h>
#include <functional>
#include <WiFi.h>
#include <WebSocketsClient.h>

enum class AssistantState {
    DISCONNECTED,
    CONNECTED,
    LISTENING,
    PROCESSING,
    SPEAKING,
    ERROR
};

using AssistantCallback = std::function<void(AssistantState state, const char* data)>;

class AssistantClient {
public:
    void begin(const char* serverUrl, uint16_t port);
    void connect();
    void disconnect();
    void startListening();
    void stopListening();
    void update();  // Call in loop
    AssistantState state() const { return _state; }
    void onEvent(AssistantCallback cb) { _callback = cb; }

private:
    WebSocketsClient _ws;
    AssistantState _state = AssistantState::DISCONNECTED;
    AssistantCallback _callback = nullptr;
    char _serverUrl[64] = {0};
    uint16_t _serverPort = 8000;
    bool _isStreaming = false;

    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    static AssistantClient* _instance;  // For static callback
    void handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    void sendAudioChunk(const int16_t* samples, size_t count);
};
```

- [ ] **Step 2: Create AssistantClient.cpp**

```cpp
#include "AssistantClient.h"
#include <ArduinoJson.h>

AssistantClient* AssistantClient::_instance = nullptr;

void AssistantClient::begin(const char* serverUrl, uint16_t port) {
    _instance = this;
    strncpy(_serverUrl, serverUrl, sizeof(_serverUrl) - 1);
    _serverPort = port;
}

void AssistantClient::connect() {
    _ws.begin(_serverUrl, _serverPort, "/ws/audio");
    _ws.onEvent(webSocketEvent);
    _ws.setReconnectInterval(5000);
    _state = AssistantState::CONNECTED;
    if (_callback) _callback(_state, "connected");
}

void AssistantClient::disconnect() {
    _ws.disconnect();
    _state = AssistantState::DISCONNECTED;
}

void AssistantClient::startListening() {
    if (_state != AssistantState::CONNECTED) return;

    // Send start message
    DynamicJsonDocument doc(128);
    doc["type"] = "start";
    String msg;
    serializeJson(doc, msg);
    _ws.sendTXT(msg);

    _state = AssistantState::LISTENING;
    _isStreaming = true;
    if (_callback) _callback(_state, "listening");
}

void AssistantClient::stopListening() {
    if (!_isStreaming) return;

    // Send end message
    DynamicJsonDocument doc(128);
    doc["type"] = "end";
    String msg;
    serializeJson(doc, msg);
    _ws.sendTXT(msg);

    _isStreaming = false;
    _state = AssistantState::PROCESSING;
    if (_callback) _callback(_state, "processing");
}

void AssistantClient::update() {
    _ws.loop();

    // TODO: Read microphone samples and send via sendAudioChunk()
    // This depends on Bruce's microphone driver
}

void AssistantClient::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (_instance) {
        _instance->handleWebSocketEvent(type, payload, length);
    }
}

void AssistantClient::handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            _state = AssistantState::CONNECTED;
            if (_callback) _callback(_state, "connected");
            break;

        case WStype_TEXT: {
            DynamicJsonDocument doc(1024);
            DeserializationError err = deserializeJson(doc, payload, length);
            if (err) return;

            const char* msgType = doc["type"];
            if (strcmp(msgType, "text") == 0) {
                const char* text = doc["text"];
                if (_callback) _callback(AssistantState::SPEAKING, text);
            } else if (strcmp(msgType, "audio") == 0) {
                // Play audio response (future: audio output)
                _state = AssistantState::SPEAKING;
            }
            break;
        }

        case WStype_DISCONNECTED:
            _state = AssistantState::DISCONNECTED;
            if (_callback) _callback(_state, "disconnected");
            break;

        default:
            break;
    }
}

void AssistantClient::sendAudioChunk(const int16_t* samples, size_t count) {
    // Send raw PCM bytes
    _ws.sendBIN((const uint8_t*)samples, count * sizeof(int16_t));
}
```

- [ ] **Step 3: Create AssistantUI.h**

```cpp
#pragma once
#include <lvgl.h>
#include "../assistant/AssistantClient.h"

class AssistantUI {
public:
    void begin(lv_obj_t* parent, AssistantClient* client);
    void update();

private:
    lv_obj_t* _container = nullptr;
    lv_obj_t* _statusLabel = nullptr;
    lv_obj_t* _userTextLabel = nullptr;
    lv_obj_t* _responseLabel = nullptr;
    AssistantClient* _client = nullptr;

    void createUI();
    void onAssistantEvent(AssistantState state, const char* data);
};
```

- [ ] **Step 4: Create AssistantUI.cpp**

```cpp
#include "AssistantUI.h"

void AssistantUI::begin(lv_obj_t* parent, AssistantClient* client) {
    _client = client;
    _container = parent;
    createUI();

    _client->onEvent([this](AssistantState state, const char* data) {
        onAssistantEvent(state, data);
    });
}

void AssistantUI::createUI() {
    lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(_container, 10, 0);

    _statusLabel = lv_label_create(_container);
    label_set_text(_statusLabel, "Conectado");
    lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0x00FF00), 0);

    _userTextLabel = lv_label_create(_container);
    lv_label_set_text(_userTextLabel, "");
    lv_obj_set_width(_userTextLabel, LV_PCT(100));
    lv_label_set_long_mode(_userTextLabel, LV_LABEL_LONG_WRAP);

    _responseLabel = lv_label_create(_container);
    lv_label_set_text(_responseLabel, "");
    lv_obj_set_width(_responseLabel, LV_PCT(100));
    lv_label_set_long_mode(_responseLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(_responseLabel, lv_color_hex(0x00BFFF), 0);
}

void AssistantUI::onAssistantEvent(AssistantState state, const char* data) {
    switch (state) {
        case AssistantState::LISTENING:
            lv_label_set_text(_statusLabel, "Ouvindo...");
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0xFFFF00), 0);
            break;
        case AssistantState::PROCESSING:
            lv_label_set_text(_statusLabel, "Pensando...");
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0xFF8800), 0);
            break;
        case AssistantState::SPEAKING:
            lv_label_set_text(_statusLabel, "Falando...");
            lv_label_set_text(_responseLabel, data);
            break;
        case AssistantState::CONNECTED:
            lv_label_set_text(_statusLabel, "Conectado");
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0x00FF00), 0);
            break;
        case AssistantState::DISCONNECTED:
            lv_label_set_text(_statusLabel, "Desconectado");
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0xFF0000), 0);
            break;
        default:
            break;
    }
}

void AssistantUI::update() {
    // Future: handle touch input for push-to-talk
}
```

- [ ] **Step 5: Commit**

```bash
git add src/watch/assistant/
git commit -m "feat(assistant): WebSocket client + conversation UI for voice pipeline"
```

---

### Task 8: Music player

**Files:**
- Create: `src/watch/music/MusicPlayer.h`
- Create: `src/watch/music/MusicPlayer.cpp`
- Create: `src/watch/music/MusicUI.h`
- Create: `src/watch/music/MusicUI.cpp`

**Interfaces:**
- Consumes: WiFi, WebSocket
- Produces: `MusicPlayer::connect(url, port)`, `MusicPlayer::play()`, `MusicPlayer::pause()`, `MusicPlayer::next()`, `MusicPlayer::prev()`

- [ ] **Step 1: Create MusicPlayer.h**

```cpp
#pragma once
#include <WebSocketsClient.h>
#include <functional>

struct TrackInfo {
    char title[128];
    char artist[128];
    bool isPlaying;
    uint32_t durationMs;
    uint32_t positionMs;
};

using MusicCallback = std::function<void(const TrackInfo& track)>;

class MusicPlayer {
public:
    void begin(const char* serverUrl, uint16_t port);
    void connect();
    void disconnect();
    void play();
    void pause();
    void next();
    void prev();
    void update();
    void onTrackChange(MusicCallback cb) { _callback = cb; }
    const TrackInfo& currentTrack() const { return _track; }

private:
    WebSocketsClient _ws;
    MusicCallback _callback = nullptr;
    TrackInfo _track = {};
    char _serverUrl[64] = {0};
    uint16_t _serverPort = 8000;

    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    static MusicPlayer* _instance;
    void handleEvent(WStype_t type, uint8_t* payload, size_t length);
    void sendCommand(const char* cmd);
};
```

- [ ] **Step 2: Create MusicPlayer.cpp**

```cpp
#include "MusicPlayer.h"
#include <ArduinoJson.h>

MusicPlayer* MusicPlayer::_instance = nullptr;

void MusicPlayer::begin(const char* serverUrl, uint16_t port) {
    _instance = this;
    strncpy(_serverUrl, serverUrl, sizeof(_serverUrl) - 1);
    _serverPort = port;
}

void MusicPlayer::connect() {
    _ws.begin(_serverUrl, _serverPort, "/ws/music");
    _ws.onEvent(webSocketEvent);
    _ws.setReconnectInterval(5000);
}

void MusicPlayer::disconnect() {
    _ws.disconnect();
}

void MusicPlayer::play() { sendCommand("play"); }
void MusicPlayer::pause() { sendCommand("pause"); }
void MusicPlayer::next() { sendCommand("next"); }
void MusicPlayer::prev() { sendCommand("prev"); }

void MusicPlayer::sendCommand(const char* cmd) {
    DynamicJsonDocument doc(128);
    doc["type"] = "command";
    doc["action"] = cmd;
    String msg;
    serializeJson(doc, msg);
    _ws.sendTXT(msg);
}

void MusicPlayer::update() {
    _ws.loop();
}

void MusicPlayer::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (_instance) _instance->handleEvent(type, payload, length);
}

void MusicPlayer::handleEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_TEXT) {
        DynamicJsonDocument doc(512);
        if (deserializeJson(doc, payload, length)) return;

        if (doc.containsKey("title")) {
            strncpy(_track.title, doc["title"] | "", sizeof(_track.title));
            strncpy(_track.artist, doc["artist"] | "", sizeof(_track.artist));
            _track.isPlaying = doc["isPlaying"] | false;
            _track.durationMs = doc["duration"] | 0;
            _track.positionMs = doc["position"] | 0;

            if (_callback) _callback(_track);
        }
    }
}
```

- [ ] **Step 3: Create MusicUI (simple player screen)**

```cpp
// MusicUI.h
#pragma once
#include <lvgl.h>
#include "../music/MusicPlayer.h"

class MusicUI {
public:
    void begin(lv_obj_t* parent, MusicPlayer* player);
    void update();

private:
    lv_obj_t* _container = nullptr;
    lv_obj_t* _titleLabel = nullptr;
    lv_obj_t* _artistLabel = nullptr;
    lv_obj_t* _playBtn = nullptr;
    lv_obj_t* _nextBtn = nullptr;
    lv_obj_t* _prevBtn = nullptr;
    MusicPlayer* _player = nullptr;
    bool _isPlaying = false;

    void createUI();
    void onTrackChange(const TrackInfo& track);
    static void playBtnCallback(lv_event_t* e);
    static void nextBtnCallback(lv_event_t* e);
    static void prevBtnCallback(lv_event_t* e);
};
```

- [ ] **Step 4: Commit**

```bash
git add src/watch/music/
git commit -m "feat(music): WebSocket music player with UI controls"
```

---

### Task 9: ANCS notifications

**Files:**
- Create: `src/watch/ancs/ANCSDisplay.h`
- Create: `src/watch/ancs/ANCSDisplay.cpp`

**Interfaces:**
- Consumes: BLE (Bruce ANCS client if available, or new implementation)
- Produces: `ANCSDisplay::begin(lv_obj_t* parent)`, `ANCSDisplay::addNotification(...)`, `ANCSDisplay::update()`

- [ ] **Step 1: Create ANCSDisplay.h**

```cpp
#pragma once
#include <lvgl.h>

struct Notification {
    char app[32];
    char title[64];
    char message[128];
    uint32_t timestamp;
};

class ANCSDisplay {
public:
    void begin(lv_obj_t* parent);
    void addNotification(const Notification& notif);
    void update();
    int unreadCount() const { return _unreadCount; }

private:
    static const int MAX_NOTIFICATIONS = 10;
    lv_obj_t* _container = nullptr;
    lv_obj_t* _list = nullptr;
    lv_obj_t* _countLabel = nullptr;
    Notification _notifications[MAX_NOTIFICATIONS] = {};
    int _notifCount = 0;
    int _unreadCount = 0;

    void createUI();
    void refreshList();
};
```

- [ ] **Step 2: Create ANCSDisplay.cpp**

```cpp
#include "ANCSDisplay.h"
#include <cstdio>

void ANCSDisplay::begin(lv_obj_t* parent) {
    _container = parent;
    createUI();
}

void ANCSDisplay::createUI() {
    lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(_container, 5, 0);

    // Header
    lv_obj_t* header = lv_label_create(_container);
    lv_label_set_text(header, "Notificações");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);

    _countLabel = lv_label_create(_container);
    lv_label_set_text(_countLabel, "0 não lidas");

    // Notification list
    _list = lv_list_create(_container);
    lv_obj_set_size(_list, LV_PCT(100), LV_SIZE_CONTENT);
}

void ANCSDisplay::addNotification(const Notification& notif) {
    if (_notifCount >= MAX_NOTIFICATIONS) {
        // Shift older notifications
        for (int i = 0; i < MAX_NOTIFICATIONS - 1; i++) {
            _notifications[i] = _notifications[i + 1];
        }
        _notifCount = MAX_NOTIFICATIONS - 1;
    }

    _notifications[_notifCount++] = notif;
    _unreadCount++;

    char countBuf[32];
    snprintf(countBuf, sizeof(countBuf), "%d não lidas", _unreadCount);
    lv_label_set_text(_countLabel, countBuf);

    refreshList();
}

void ANCSDisplay::refreshList() {
    lv_obj_clean(_list);

    for (int i = _notifCount - 1; i >= 0; i--) {
        lv_obj_t* btn = lv_list_add_btn(_list, LV_SYMBOL_BELL, _notifications[i].title);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A1A2E), 0);

        lv_obj_t* msgLabel = lv_label_create(btn);
        lv_label_set_text(msgLabel, _notifications[i].message);
        lv_label_set_long_mode(msgLabel, LV_LABEL_LONG_DOT);
        lv_obj_set_width(msgLabel, LV_PCT(80));
    }
}

void ANCSDisplay::update() {
    // Future: tap to expand, swipe to dismiss
}
```

- [ ] **Step 3: Commit**

```bash
git add src/watch/ancs/
git commit -m "feat(ancs): notification display with list UI"
```

---

### Task 10: GPS via BLE

**Files:**
- Create: `src/watch/gps_ble/GPSClient.h`
- Create: `src/watch/gps_ble/GPSClient.cpp`

**Interfaces:**
- Consumes: BLE (Bruce BLE stack)
- Produces: `GPSClient::begin()`, `GPSClient::update()`, `GPSClient::location()`

- [ ] **Step 1: Create GPSClient.h**

```cpp
#pragma once

struct Location {
    double latitude;
    double longitude;
    float altitude;
    bool valid;
};

class GPSClient {
public:
    void begin();
    void update();
    Location location() const { return _location; }

private:
    Location _location = {0, 0, 0, false};

    // BLE GATT client for iOS Location service
    // UUID: 00001813-0000-1000-8000-00805F9B34FB (iOS Location)
    // Note: iOS doesn't expose location via standard BLE GATT
    // This is a placeholder for when we implement a companion app
    // or use server-side IP geolocation as fallback

    void tryConnectBLE();
    void parseLocationData(const uint8_t* data, size_t len);
};
```

- [ ] **Step 2: Create GPSClient.cpp**

```cpp
#include "GPSClient.h"

void GPSClient::begin() {
    _location = {0, 0, 0, false};
    // BLE GPS connection would go here
    // For now, rely on server IP geolocation
}

void GPSClient::update() {
    // Future: poll BLE location characteristic
    // For now, no-op
}

void GPSClient::tryConnectBLE() {
    // TODO: Implement BLE GATT client for location
    // This requires a companion iOS app or specific BLE service
}

void GPSClient::parseLocationData(const uint8_t* data, size_t len) {
    // TODO: Parse location from BLE characteristic
    // Format depends on companion app implementation
}
```

- [ ] **Step 3: Commit**

```bash
git add src/watch/gps_ble/
git commit -m "feat(gps): BLE GPS client skeleton (server fallback for now)"
```

---

### Task 11: Energy management

**Files:**
- Create: `src/watch/energy/PowerManager.h`
- Create: `src/watch/energy/PowerManager.cpp`

**Interfaces:**
- Consumes: ESP32 deep sleep APIs, display driver
- Produces: `PowerManager::begin()`, `PowerManager::update()`, `PowerManager::enterSleep()`

- [ ] **Step 1: Create PowerManager.h**

```cpp
#pragma once

enum class PowerMode {
    WATCH,      // Display on, BLE on, WiFi off
    ASSISTANT,  // Display on, BLE on, WiFi on, Mic on
    HACKER,     // Display on, BLE on, WiFi/LoRa per tool
    SLEEP       // Deep sleep
};

class PowerManager {
public:
    void begin();
    void update();
    void setMode(PowerMode mode);
    PowerMode currentMode() const { return _mode; }
    void enterSleep(uint64_t sleepTimeUs = 0);  // 0 = indefinite

private:
    PowerMode _mode = PowerMode::WATCH;
    uint32_t _lastActivityTime = 0;
    uint32_t _dimTimeoutMs = 10000;   // 10 seconds
    uint32_t _sleepTimeoutMs = 30000; // 30 seconds

    void dimDisplay();
    void turnOffDisplay();
    void enableWiFi();
    void disableWiFi();
};
```

- [ ] **Step 2: Create PowerManager.cpp**

```cpp
#include "PowerManager.h"
#include <Arduino.h>
#include <esp_sleep.h>

void PowerManager::begin() {
    _lastActivityTime = millis();
    _mode = PowerMode::WATCH;

    // Default: WiFi off, BLE on
    disableWiFi();
}

void PowerManager::update() {
    uint32_t now = millis();
    uint32_t idle = now - _lastActivityTime;

    if (_mode == PowerMode::WATCH) {
        if (idle > _sleepTimeoutMs) {
            turnOffDisplay();
            // Enter light sleep or deep sleep
        } else if (idle > _dimTimeoutMs) {
            dimDisplay();
        }
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
            // WiFi/LoRa enabled per tool
            break;
        case PowerMode::SLEEP:
            enterSleep();
            break;
    }
}

void PowerManager::enterSleep(uint64_t sleepTimeUs) {
    // Configure wake sources
    esp_sleep_enable_gpio_wakeup();
    // TODO: Configure BMA423 wake, touch wake, button wake

    if (sleepTimeUs > 0) {
        esp_sleep_enable_timer_wakeup(sleepTimeUs);
    }

    esp_deep_sleep_start();
}

void PowerManager::dimDisplay() {
    // Reduce brightness via PWM or LVGL backlight
    // TODO: Use Bruce's display brightness API
}

void PowerManager::turnOffDisplay() {
    // TODO: Use Bruce's display off API
}

void PowerManager::enableWiFi() {
    // TODO: Use Bruce's WiFi API
}

void PowerManager::disableWiFi() {
    // TODO: Use Bruce's WiFi API
}
```

- [ ] **Step 3: Commit**

```bash
git add src/watch/energy/
git commit -m "feat(energy): power manager with sleep/dim/WiFi policies"
```

---

## Phase 4: Server Updates

### Task 12: Add /ws/music endpoint

**Files:**
- Modify: `server/main.py`

**Interfaces:**
- Consumes: WebSocket connection from firmware
- Produces: `/ws/music` endpoint

- [ ] **Step 1: Add music WebSocket endpoint**

In `server/main.py`, add:

```python
@app.websocket("/ws/music")
async def music_ws(websocket: WebSocket):
    await websocket.accept()
    try:
        while True:
            data = await websocket.receive_json()
            if data.get("type") == "command":
                action = data.get("action")
                if action == "play":
                    # Call music play tool
                    result = await music_play()
                elif action == "pause":
                    result = await music_pause()
                elif action == "next":
                    result = await music_next()
                elif action == "prev":
                    result = await music_prev()
                else:
                    result = {"error": f"Unknown action: {action}"}

                # Send track info back
                track_info = get_current_track()
                await websocket.send_json(track_info)
    except WebSocketDisconnect:
        pass
```

- [ ] **Step 2: Add get_current_track() helper**

```python
async def get_current_track() -> dict:
    # Placeholder — integrate with ytmusicapi
    return {
        "title": "No track",
        "artist": "Unknown",
        "isPlaying": False,
        "duration": 0,
        "position": 0
    }
```

- [ ] **Step 3: Test with test_client.py**

Add a music test to `server/scripts/test_client.py`:

```python
async def test_music_ws():
    async with websockets.connect("ws://localhost:8000/ws/music") as ws:
        await ws.send(json.dumps({"type": "command", "action": "play"}))
        response = await ws.recv()
        print(f"Music response: {response}")
```

- [ ] **Step 4: Commit**

```bash
git add server/
git commit -m "feat(server): add /ws/music WebSocket endpoint"
```

---

### Task 13: Add /api/notifications and /api/gps

**Files:**
- Modify: `server/main.py`

- [ ] **Step 1: Add notifications endpoint**

```python
@app.post("/api/notifications")
async def receive_notification(notif: dict):
    """Receive notification from firmware for logging."""
    # Store in memory or database
    notifications.append({
        "app": notif.get("app", ""),
        "title": notif.get("title", ""),
        "message": notif.get("message", ""),
        "timestamp": notif.get("timestamp", time.time())
    })
    # Keep last 50
    if len(notifications) > 50:
        notifications.pop(0)
    return {"status": "ok"}
```

- [ ] **Step 2: Add GPS endpoint**

```python
@app.get("/api/gps")
async def get_gps():
    """Return last known location (from device or IP fallback)."""
    if last_known_location:
        return last_known_location
    # IP-based fallback
    return {"latitude": 0, "longitude": 0, "source": "none"}
```

- [ ] **Step 3: Commit**

```bash
git add server/
git commit -m "feat(server): add /api/notifications and /api/gps endpoints"
```

---

## Phase 5: Integration & Polish

### Task 14: Wire everything in main.cpp

**Files:**
- Modify: `src/main.cpp` (Bruce's entry point)

- [ ] **Step 1: Add includes and globals**

```cpp
// At top of main.cpp, after Bruce includes:
#include "watch/ScreenManager.h"
#include "watch/watchface/WatchFace.h"
#include "watch/avatar/Avatar.h"
#include "watch/assistant/AssistantClient.h"
#include "watch/assistant/AssistantUI.h"
#include "watch/music/MusicPlayer.h"
#include "watch/music/MusicUI.h"
#include "watch/gestures/GestureEngine.h"
#include "watch/ancs/ANCSDisplay.h"
#include "watch/energy/PowerManager.h"

static ScreenManager screenMgr;
static WatchFace watchFace;
static AssistantClient assistant;
static AssistantUI assistantUI;
static MusicPlayer music;
static MusicUI musicUI;
static GestureEngine gestures;
static ANCSDisplay ancs;
static PowerManager power;
```

- [ ] **Step 2: Initialize in setup()**

After Bruce's init code:

```cpp
// Initialize b0r4-watch modules
screenMgr.begin(lv_scr_act());

// Watchface
watchFace.begin(screenMgr.getScreen(Screen::WATCHFACE));

// Assistant
assistant.begin("192.168.1.100", 8000);  // Server IP
assistant.connect();
assistantUI.begin(screenMgr.getScreen(Screen::ASSISTANT), &assistant);

// Music
music.begin("192.168.1.100", 8000);
music.connect();
musicUI.begin(screenMgr.getScreen(Screen::MUSIC), &music);

// Gestures
gestures.begin();
gestures.onAction([](const GestureEvent& event) {
    switch (event.action) {
        case ActionType::OPEN_ASSISTANT:
            screenMgr.switchTo(Screen::ASSISTANT);
            assistant.startListening();
            break;
        case ActionType::OPEN_HACKER:
            screenMgr.switchTo(Screen::HACKER);
            break;
        case ActionType::MUSIC_TOGGLE:
            if (music.currentTrack().isPlaying) music.pause();
            else music.play();
            break;
        case ActionType::DEEP_SLEEP:
            power.enterSleep();
            break;
        default:
            break;
    }
});

// ANCS (placeholder until BLE is connected)
ancs.begin(screenMgr.getScreen(Screen::NOTIFICATIONS));

// Power
power.begin();
```

- [ ] **Step 3: Update loop()**

```cpp
void loop() {
    lv_timer_handler();
    gestures.update();
    assistant.update();
    music.update();
    power.update();
    watchFace.update();
    delay(5);
}
```

- [ ] **Step 4: Commit**

```bash
git add src/main.cpp
git commit -m "feat: wire all b0r4-watch modules in main entry point"
```

---

### Task 15: Integration test on hardware

**Files:**
- None (testing only)

- [ ] **Step 1: Flash to T-Watch S3**

```bash
pio run -e twatch-s3 --target upload
```

- [ ] **Step 2: Verify watchface displays**

Open serial monitor, verify clock updates every second.

- [ ] **Step 3: Test swipe navigation**

Swipe left/right between screens. Verify all 5 screens accessible.

- [ ] **Step 4: Test voice assistant**

Open server on PC, connect watch to same WiFi, test voice pipeline.

- [ ] **Step 5: Test gesture engine**

Shake watch left → assistant opens. Shake right → hacker tools open.

- [ ] **Step 6: Test battery drain**

Let watch sit for 1 hour in WATCH mode, measure battery consumption.

- [ ] **Step 7: Document results**

Update `docs/integration-test-results.md` with findings.

- [ ] **Step 8: Commit**

```bash
git add docs/
git commit -m "docs: integration test results from first hardware test"
```

---

## Summary

| Phase | Tasks | Effort |
|---|---|---|
| Phase 1: Setup | Tasks 1-2 | 1 day |
| Phase 2: Watchface | Tasks 3-5 | 2-3 days |
| Phase 3: Core Features | Tasks 6-11 | 5-7 days |
| Phase 4: Server | Tasks 12-13 | 1 day |
| Phase 5: Integration | Tasks 14-15 | 2-3 days |
| **Total** | **15 tasks** | **~2-3 weeks** |

## What Could Go Wrong

1. **Bruce's display driver conflicts with LVGL** — Bruce might use TFT_eSPI directly. May need to adapt watchface to use TFT_eSPI instead of LVGL, or find Bruce's LVGL integration.
2. **BMA423 accelerometer API different in Bruce** — Gesture engine may need different sensor reading calls.
3. **WebSocket lib not included in Bruce** — May need to add `arduinoWebSockets` to platformio.ini.
4. **BLE stack conflicts** — Bruce's BLE and ANCS may need careful integration.
5. **Server IP hardcoded** — Need config mechanism (bruce.conf or similar).
