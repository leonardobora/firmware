# Design Spec: Eyes Watch Face + LVGL PC Simulator

**Date:** 2026-08-17
**Author:** Leonardo Bora + AI
**Status:** Approved
**Scope:** Watch face digital com olhos animados + LVGL PC Simulator para desenvolvimento sem hardware

---

## 1. Overview

Replace the current ASCII avatar (Phase 0) with a full **animated eyes watch face** on the LILYGO T-Watch S3 (240x240, ESP32-S3). Two large eyes dominate the screen center, react to the accelerometer (gaze tracking), and exhibit autonomous personality behaviors (blink, saccades, sleepy mode, easter egg "chapado"). Time and status indicators are displayed minimally around the eyes.

An **LVGL PC Simulator** environment enables developing and testing the entire watch face on desktop without the physical hardware.

## 2. Goals

1. Create a visually striking, minimal-tech watch face with animated eyes as the primary element
2. Eyes react to wrist movement via BMA423 accelerometer (gaze tracking)
3. Eyes have autonomous personality (blink, saccades, sleepy, surprise)
4. Easter egg "chapado" mode (red droopy eyes) triggered by shake 3x
5. Time + date + status indicators displayed minimally
6. Gesture-based navigation to secondary screens (notifications, apps, settings)
7. LVGL PC Simulator for desktop development (SDL2 driver)
8. Same codebase compiles for both PC simulator and real hardware

## 3. Display Layout

**Screen:** 240x240 pixels, ST7789, round (corners clipped)

```
┌─────────────────────────────┐
│       14:32    🔋 87%       │  ← Time (small, top, alpha 80%)
│                             │
│        ╭─────╮ ╭─────╮     │
│        │ ◉   │ │   ◉ │     │  ← Eyes (canvas, center, dominant)
│        ╰─────╯ ╰─────╯     │
│                             │
│       seg 17 ago 2026       │  ← Date (tiny, bottom, alpha 50%)
└─────────────────────────────┘
```

**Color palette:**
- Background: `#000000` (pure black)
- Sclera: `#FFFFFF` (white)
- Iris: configurable, default `#4A90D9` (blue)
- Pupil: `#000000` (black)
- Time text: `#FFFFFF` at 80% alpha
- Date text: `#FFFFFF` at 50% alpha
- Status icons: `#FFFFFF` at 40% alpha
- Easter egg iris: `#CC3333` (red)

## 4. Eye System

### 4.1 Eye Composition (per eye)

Each eye is rendered on an LVGL canvas as 4 layered circles:

| Layer | Shape | Size | Color | Purpose |
|-------|-------|------|-------|---------|
| Sclera | Filled circle | 38px diameter | White | White of the eye |
| Iris | Filled circle | 20px diameter | Configurable | Colored iris |
| Pupil | Filled circle | 8px diameter | Black | Pupil position varies with gaze |
| Eyelid | Arc | Covers top portion | Black (matches bg) | Closes during blink |

### 4.2 Gaze Tracking

Uses BMA423 accelerometer data to move pupil position:

```
pupila_x = accelX * sensitivity  (range: -6px to +6px)
pupila_y = accelY * sensitivity  (range: -4px to +4px)
```

- Wrist flat → pupils centered
- Wrist tilted left → pupils move left
- Wrist raised → pupils move up

**Smoothing:** Linear interpolation (lerp) with factor 0.15 per frame for organic movement.

### 4.3 Personality Behaviors

| Behavior | Frequency | Effect |
|----------|-----------|--------|
| **Blink** | Every ~5s + 0-500ms jitter | Eyelid closes and opens smoothly (ported from existing `BlinkModifier`) |
| **Saccade** | Every 3-8s (random) | Pupil jumps rapidly to a random point, then returns to gaze position |
| **Dwell** | When still >3s | Eyes "fixate" on a point, as if thinking |
| **Sleepy** | When idle >30s | Eyelids lower partially, pupil constricts slightly |
| **Surprise** | On rapid shake | Eyes widen (iris shrinks, pupil expands) |

### 4.4 Easter Egg: "Chapado" Mode

Triggered by shaking the watch 3 times rapidly.

| Property | Normal | Chapado |
|----------|--------|---------|
| Iris color | Blue | Red (`#CC3333`) |
| Eyelid position | Open | 30% closed permanently |
| Pupil movement speed | Normal | Slow, floating |
| Saccade frequency | Every 3-8s | Every 8-15s |
| Blink speed | ~200ms | ~400ms (sluggish) |
| Overall feel | Alert, responsive | Heavy-lidded, slow |

## 5. Time Display

- **Font:** Mono small (~16px), LVGL built-in mono font
- **Format:** `HH:MM` (24h)
- **Position:** Top center
- **Color:** White, 80% alpha
- **Update:** Once per minute (timer 60s)
- **Transition:** Subtle fade when minute changes

## 6. Date Display

- **Font:** Tiny (~12px)
- **Format:** `seg 17 ago` (abbreviated, no year)
- **Position:** Bottom center
- **Color:** White, 50% alpha
- **Update:** Once per hour

## 7. Status Indicators

Positioned at top right, next to time:

| Indicator | Icon | Update Frequency |
|-----------|------|-----------------|
| Battery | Battery icon + % | Every 5 minutes |
| Wi-Fi | Signal icon (on/off) | On connect/disconnect |
| BLE | Bluetooth icon (on/off) | On connect/disconnect |

All icons: ~10px, 40% alpha — functional but not attention-grabbing.

## 8. Gesture Navigation

### 8.1 Screen Map

```
         ┌──────────────┐
         │  Watch Face   │  ← base screen (eyes + time)
         │  (base)       │
         └──────┬───────┘
                │
    ┌───────────┼───────────┐
    ▼           ▼           ▼
┌────────┐ ┌────────┐ ┌────────┐
│ Notif.  │ │ Apps   │ │ Config │
│ (swipe↑)│ │(swipe→)│ │(swipe↓)│
└────────┘ └────────┘ └────────┘
```

### 8.2 Gesture Map

| Trigger | Source | Action |
|---------|--------|--------|
| Wrist raise | BMA423 wake | Wake screen → show watch face |
| Swipe up | Touch | Notifications screen |
| Swipe right | Touch | App launcher |
| Swipe down | Touch | Quick settings |
| Shake 2x | BMA423 (custom shake detection) | Open voice assistant |
| Shake 3x | BMA423 (custom shake detection) | Easter egg "chapado" |
| Double-click power | Button | Skip music / custom action |
| Long press power | Button | Turn off display (deep sleep) |
| Tap screen | Touch | Toggle time+status visibility |

**Shake detection detail:** The GestureEngine counts shakes within a 2-second window. Each shake is detected by accelerometer magnitude exceeding a threshold (±1.5g) with direction change. If 2 shakes are detected → voice assistant. If 3 shakes → easter egg. The counter resets after 2s of no shake activity. This prevents accidental triggers.

### 8.3 Transitions

- All screen transitions: 200ms fade in/out
- Eyes canvas continues rendering in background
- Swipe back to watch face: eyes "follow" the finger direction

### 8.4 PC Simulator Input Mapping

| Keyboard Key | Simulated Gesture |
|-------------|-------------------|
| Arrow keys (←→↑↓) | Touch swipes |
| Space | Tap |
| Enter | Shake |
| Backspace | Long press power |

## 9. Power Management

### 9.1 Power States

| State | Description | FPS | Radios | Current |
|-------|-------------|-----|--------|---------|
| **FULL ON** | Eyes animated, all sensors active | 30 | Wi-Fi + BLE on | ~80mA |
| **IDLE** | Eyes sleepy, low-power sensors | 10 | Wi-Fi off, BLE low | ~30mA |
| **AOD** | Time + static eyes, minimal refresh | 1 | All radios off | ~5mA |
| **DEEP SLEEP** | Display off, RTC active | 0 | All off | ~0.5mA |

### 9.2 Auto-Transitions

| Event | From → To | Delay |
|-------|-----------|-------|
| Wrist raised | Deep sleep → Full ON | Immediate |
| No interaction | Full ON → Idle | 10s |
| No interaction | Idle → AOD | 30s |
| No interaction | AOD → Deep sleep | 60s |
| Touch/gesture | Any → Full ON | Immediate |

### 9.3 PC Simulator Behavior

Power management is mocked on PC — always runs in "Full ON" mode. Transition logic is tested via unit tests with mocked timers.

## 10. Architecture

### 10.1 Module Structure

```
firmware/src/
├── main.cpp                    # Entry point (hardware)
├── main_desktop.cpp            # Entry point (simulator, SDL2)
├── eyes/
│   ├── EyeRenderer.cpp/h       # Canvas drawing (sclera, iris, pupil, eyelid)
│   ├── EyeState.cpp/h          # Gaze position, blink state, personality state
│   ├── Personality.cpp/h       # Blink, saccade, dwell, sleepy behaviors
│   └── EasterEgg.cpp/h         # Chapado mode
├── watchface/
│   ├── WatchFace.cpp/h         # Layout manager (positions all elements)
│   ├── TimeDisplay.cpp/h       # Time label + update timer
│   ├── DateDisplay.cpp/h       # Date label + update timer
│   └── StatusIcons.cpp/h       # Battery, Wi-Fi, BLE icons
├── input/
│   ├── GestureEngine.cpp/h     # Maps triggers → actions (ADR 0007)
│   ├── InputAdapter.cpp/h      # Platform abstraction (touch vs keyboard)
│   └── gestures.json           # User-customizable gesture map
├── power/
│   └── PowerManager.cpp/h      # State machine (FULL_ON → IDLE → AOD → SLEEP)
├── avatar/                     # Legacy ASCII avatar (kept for reference)
│   ├── Avatar.cpp/h
│   └── modifiers/
└── platform/
    ├── hal_esp32.cpp/h          # ESP32-S3 HAL (accel, touch, display)
    └── hal_desktop.cpp/h       # Desktop HAL (SDL2, keyboard, mock sensors)
```

### 10.2 Key Interfaces

```cpp
// EyeState — shared between all eye-related modules
struct EyeState {
    float gazeX, gazeY;      // -1.0 to 1.0 (normalized)
    float blinkProgress;      // 0.0 (open) to 1.0 (closed)
    float irisScale;          // 1.0 normal, <1 surprise
    float pupilScale;         // 1.0 normal, >1 surprise
    bool isChapado;           // easter egg mode
};

// InputAdapter — platform abstraction
class InputAdapter {
public:
    virtual GestureEvent poll() = 0;  // returns gesture or NONE
    virtual bool hasAccel() const = 0; // false on PC
    virtual AccelData readAccel() = 0; // returns mock data on PC
};

// DesktopInputAdapter (concrete implementation for PC)
// Maps SDL2 keyboard events to GestureEvent:
//   Arrow keys → SWIPE_UP/DOWN/LEFT/RIGHT
//   Space → TAP
//   Enter → SHAKE (single shake event)
//   Backspace → LONG_PRESS_POWER
// Accel data is mocked: arrows also tilt the gaze direction
// (holding left arrow = wrist tilted left = gaze moves left)

// PowerManager — state machine
enum PowerState { FULL_ON, IDLE, AOD, DEEP_SLEEP };
class PowerManager {
public:
    PowerState getState() const;
    void onInteraction();    // resets idle timer
    void update(uint32_t ms); // checks timeouts, transitions state
};
```

### 10.3 PlatformIO Environments

```ini
[env:twatch-s3]
; Existing hardware target
platform = espressif32@6.3.0
framework = arduino
board = LilyGoWatch-S3
build_flags = -DBOARD_HAS_PSRAM ...

[env:simulator]
; NEW: Desktop simulator
platform = native
framework = arduino  ; or plain C++
build_flags = -DSIMULATOR
lib_deps =
    lvgl/lvgl @ 8.4.0
    lvgl/lv_drivers @ 0.9.2  ; SDL2 display + input drivers
    ; SDL2 must be installed system-wide: winget install SDL2
src_filter = +<main_desktop.cpp> +<eyes/> +<watchface/> +<input/> +<power/> +<platform/>
```

**Note:** The `platform = native` environment compiles C++ directly for the host OS. SDL2 libraries are linked dynamically. The `lv_drivers` package provides the SDL2 display driver (`sdl.c`) and input driver that we initialize in `main_desktop.cpp`.

## 11. LVGL PC Simulator Setup

### 11.1 Dependencies

- SDL2 (`winget install SDL2` on Windows)
- LVGL 8.4.0 (already in project)
- `lv_drivers` repository (provides `lv_drv_conf.h` and SDL2 display/mouse drivers)
- PlatformIO with `platform = native`

**SDL2 Integration:** Uses the official LVGL SDL2 driver from `lv_drivers/sdl/sdl.h`. This provides `disp_drv` (display) and `indev_drv` (input device) initialization. We write a thin adapter layer on top that translates SDL2 keyboard events to our `GestureEvent` type.

### 11.2 What Runs on PC vs Hardware

| Component | PC (Simulator) | Hardware |
|-----------|---------------|----------|
| Eye rendering | SDL2 window (240x240) | TFT_eSPI display |
| Gaze input | Keyboard arrows | BMA423 accelerometer |
| Touch input | Keyboard (space/arrows) | Capacitive touch |
| Time/RTC | System clock | Internal RTC |
| Power management | Mocked (always FULL_ON) | Real power states |
| BLE/Wi-Fi | Mocked (disconnected) | Real radios |

### 11.3 Running the Simulator

```bash
cd firmware
pio run -e simulator
# Opens a 240x240 window with the watch face
# Arrow keys = swipes, Space = tap, Enter = shake
```

## 12. Implementation Phases

| Phase | Description | Depends On |
|-------|-------------|------------|
| **P0: Simulator Setup** | LVGL + SDL2 environment, compile and run empty 240x240 window | Nothing |
| **P1: Eye Renderer** | Draw static eyes on canvas (sclera + iris + pupil) | P0 |
| **P2: Gaze Tracking** | Pupils move with keyboard input (simulated accelerometer) | P1 |
| **P3: Personality** | Blink, saccade, dwell behaviors | P2 |
| **P4: Time Display** | Hour + date labels, update timers | P0 |
| **P5: Easter Egg** | Chapado mode (shake 3x → red eyes) | P3 |
| **P6: Navigation** | Gesture engine, screen transitions | P0, P4 |
| **P7: Status Icons** | Battery, Wi-Fi, BLE indicators | P4 |
| **P8: Power Manager** | State machine with auto-transitions | P6 |
| **P9: Hardware Port** | Port HAL to ESP32-S3, test on real device | All above + hardware |

**P0-P5 can be developed entirely on PC simulator.**
**P6-P8 can be partially tested on PC (with mocked inputs).**
**P9 requires physical hardware.**

## 13. Success Criteria

- [ ] Watch face renders on PC simulator at 30FPS
- [ ] Eyes follow keyboard input (gaze tracking)
- [ ] Blink, saccade, sleepy behaviors work autonomously
- [ ] Easter egg "chapado" activates with 3 shakes
- [ ] Time displays and updates correctly
- [ ] Gesture navigation between screens works
- [ ] Same code compiles for both simulator and ESP32-S3
- [ ] On hardware: eyes react to real accelerometer data
- [ ] On hardware: power management transitions work

## 14. Out of Scope (This Spec)

- Notification screen content (ANCS — ADR 0006)
- App launcher content (just the screen shell)
- Settings screen content
- Voice assistant integration
- Music control
- Home automation
- LoRa radio
- GPS

These are handled by future specs.
