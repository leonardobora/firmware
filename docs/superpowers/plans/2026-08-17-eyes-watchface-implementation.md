# Eyes Watch Face + LVGL PC Simulator — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an animated eyes watch face with LVGL PC Simulator, so the full UI can be developed and tested without physical hardware.

**Architecture:** LVGL 8.4.0 renders two animated eyes on a canvas (gaze tracking + personality behaviors) plus time/date labels. A PlatformIO `native` environment links SDL2 via `lv_drivers` for desktop rendering. The same source code compiles for both PC (simulator) and ESP32-S3 (hardware).

**Tech Stack:** C++, LVGL 8.4.0, lv_drivers (SDL2), PlatformIO, SDL2

## Global Constraints

- Display: 240×240 pixels, ST7789 (hardware) / SDL2 window (simulator)
- LVGL version: 8.4.0 (already in `platformio.ini`)
- Framework: Arduino (for code sharing between environments)
- PlatformIO environments: `twatch-s3` (existing) + `simulator` (new)
- SDL2 installed system-wide (`winget install SDL2`)
- All source in `firmware/src/`
- Legacy `avatar/` directory kept for reference, not modified

---

## File Structure

```
firmware/src/
├── main.cpp                    # EXISTING — hardware entry point (modify later for P9)
├── main_desktop.cpp            # NEW — simulator entry point (SDL2 + LVGL init)
├── eyes/
│   ├── EyeRenderer.cpp         # NEW — canvas drawing (sclera, iris, pupil, eyelid)
│   ├── EyeRenderer.h           # NEW
│   ├── EyeState.cpp            # NEW — gaze position, blink, personality state
│   ├── EyeState.h              # NEW
│   ├── Personality.cpp         # NEW — blink, saccade, dwell, sleepy behaviors
│   ├── Personality.h           # NEW
│   └── EasterEgg.cpp/h         # NEW — chapado mode (future, after P3)
├── watchface/
│   ├── WatchFace.cpp           # NEW — layout manager (positions all elements)
│   ├── WatchFace.h             # NEW
│   ├── TimeDisplay.cpp         # NEW — time label + update timer
│   ├── TimeDisplay.h           # NEW
│   ├── DateDisplay.cpp         # NEW — date label + update timer
│   └── DateDisplay.h           # NEW
├── input/
│   ├── GestureEngine.h         # NEW — gesture types + engine interface
│   ├── GestureEngine.cpp       # NEW
│   ├── DesktopInputAdapter.cpp # NEW — keyboard → gesture mapping
│   └── DesktopInputAdapter.h   # NEW
├── platform/
│   └── hal.h                   # NEW — HAL abstraction (display, input, sensors)
```

---

## Task 1: Simulator Environment — Empty Window

**Goal:** Get LVGL rendering via SDL2 on desktop. Empty 240×240 black window.

**Files:**
- Create: `firmware/src/main_desktop.cpp`
- Create: `firmware/src/platform/hal.h`
- Modify: `firmware/platformio.ini` (add `[env:simulator]`)

**Interfaces:**
- Produces: `main_desktop.cpp` — entry point that inits SDL2 + LVGL + runs main loop
- Produces: `hal.h` — `#ifdef SIMULATOR` / `#ifdef ARDUINO` abstraction macros

- [ ] **Step 1: Add simulator environment to platformio.ini**

Append to `firmware/platformio.ini`:

```ini
[env:simulator]
platform = native
framework = arduino
build_flags =
    -DSIMULATOR
    -DLV_CONF_INCLUDE_SIMPLE
    -I../.pio/libdeps/twatch-s3/lvgl
lib_deps =
    lvgl/lvgl @ 8.4.0
    lvgl/lv_drivers @ 0.9.2
src_filter = +<main_desktop.cpp> +<platform/>
```

- [ ] **Step 2: Create HAL abstraction header**

Create `firmware/src/platform/hal.h`:

```cpp
#pragma once

#ifdef SIMULATOR
  // Desktop: SDL2 via lv_drivers
  #include "lvgl.h"
  #include "lv_drivers/sdl/sdl.h"
  #define HAL_DISPLAY_WIDTH  240
  #define HAL_DISPLAY_HEIGHT 240
#else
  // Hardware: TFT_eSPI
  #include "lvgl.h"
  #include <TFT_eSPI.h>
  #include "Setup212_LilyGo_T_Watch_S3.h"
  #define HAL_DISPLAY_WIDTH  240
  #define HAL_DISPLAY_HEIGHT 240
#endif
```

- [ ] **Step 3: Create main_desktop.cpp**

Create `firmware/src/main_desktop.cpp`:

```cpp
#ifdef SIMULATOR

#include "platform/hal.h"
#include <SDL2/SDL.h>

static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;
static lv_color_t buf1[240 * 40];

// SDL2 display driver callbacks (from lv_drivers)
static void sdl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *buf) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    lv_disp_t *disp = lv_disp_get_default();
    SDL_Surface *surface = (SDL_Surface *)disp->driver->user_data;
    SDL_LockSurface(surface);
    uint8_t *pixels = (uint8_t *)surface->pixels;
    uint32_t bpp = surface->format->BytesPerPixel;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            lv_color_t c = buf[y * w + x];
            uint32_t offset = ((area->y1 + y) * 240 + (area->x1 + x)) * bpp;
            pixels[offset + 0] = c.ch.blue;
            pixels[offset + 1] = c.ch.green;
            pixels[offset + 2] = c.ch.red;
        }
    }
    SDL_UnlockSurface(surface);
    lv_disp_flush_ready(drv);
}

int main() {
    // Init SDL2
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("b0r4-watch",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        240, 240, SDL_WINDOW_SHOWN);
    SDL_Surface *surface = SDL_GetWindowSurface(window);

    // Init LVGL
    lv_init();
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, 240 * 40);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = sdl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = surface;
    lv_disp_drv_register(&disp_drv);

    // Create a test label to verify rendering
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "b0r4-watch\nSimulator OK");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // Main loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }
        lv_timer_handler();
        SDL_UpdateWindowSurface(window);
        SDL_Delay(16); // ~60 FPS cap
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

#endif // SIMULATOR
```

- [ ] **Step 4: Verify simulator compiles and runs**

Run: `pio run -e simulator`
Expected: Compiles without errors

Run: `pio run -e simulator -t upload` (or just run the binary)
Expected: Opens a 240×240 window with "b0r4-watch\nSimulator OK" in white text on black

- [ ] **Step 5: Commit**

```bash
git add firmware/platformio.ini firmware/src/main_desktop.cpp firmware/src/platform/hal.h
git commit -m "feat: add LVGL PC simulator environment with SDL2"
```

---

## Task 2: EyeRenderer — Static Eyes on Canvas

**Goal:** Draw two static eyes (sclera + iris + pupil) centered on screen using LVGL canvas.

**Files:**
- Create: `firmware/src/eyes/EyeRenderer.h`
- Create: `firmware/src/eyes/EyeRenderer.cpp`
- Modify: `firmware/src/main_desktop.cpp` (replace test label with EyeRenderer)

**Interfaces:**
- Consumes: LVGL display driver from Task 1
- Produces: `EyeRenderer` class with `create(parent)` and `render(state)` methods

- [ ] **Step 1: Create EyeRenderer header**

Create `firmware/src/eyes/EyeRenderer.h`:

```cpp
#pragma once
#include "lvgl.h"

struct EyeRenderState {
    float gazeX;        // -1.0 to 1.0 (normalized pupil offset)
    float gazeY;        // -1.0 to 1.0
    float blinkProgress; // 0.0 (open) to 1.0 (closed)
    bool isChapado;     // easter egg mode
};

class EyeRenderer {
public:
    void create(lv_obj_t *parent);
    void render(const EyeRenderState &state);

private:
    void drawEye(lv_coord_t cx, lv_coord_t cy, const EyeRenderState &state);
    void clearCanvas();

    lv_obj_t *canvas_ = nullptr;
    lv_color_t *canvas_buf_ = nullptr;
    lv_coord_t canvas_w_ = 240;
    lv_coord_t canvas_h_ = 240;

    // Eye geometry (centered on canvas)
    static constexpr lv_coord_t LEFT_EYE_CX  = 80;
    static constexpr lv_coord_t LEFT_EYE_CY  = 120;
    static constexpr lv_coord_t RIGHT_EYE_CX = 160;
    static constexpr lv_coord_t RIGHT_EYE_CY = 120;
    static constexpr lv_coord_t SCLERA_R     = 19;  // 38px diameter
    static constexpr lv_coord_t IRIS_R       = 10;  // 20px diameter
    static constexpr lv_coord_t PUPIL_R      = 4;   // 8px diameter
    static constexpr lv_coord_t GAZE_MAX_PX  = 6;   // max pupil offset in pixels
};
```

- [ ] **Step 2: Create EyeRenderer implementation**

Create `firmware/src/eyes/EyeRenderer.cpp`:

```cpp
#include "EyeRenderer.h"
#include <cmath>

void EyeRenderer::create(lv_obj_t *parent) {
    canvas_buf_ = (lv_color_t *)malloc(canvas_w_ * canvas_h_ * sizeof(lv_color_t));
    canvas_ = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas_, canvas_buf_, canvas_w_, canvas_h_,
                         LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas_, LV_ALIGN_CENTER, 0, 0);
    clearCanvas();
}

void EyeRenderer::clearCanvas() {
    lv_canvas_fill_bg(canvas_, lv_color_black(), LV_OPA_COVER);
}

void EyeRenderer::render(const EyeRenderState &state) {
    clearCanvas();
    drawEye(LEFT_EYE_CX, LEFT_EYE_CY, state);
    drawEye(RIGHT_EYE_CX, RIGHT_EYE_CY, state);
}

void EyeRenderer::drawEye(lv_coord_t cx, lv_coord_t cy, const EyeRenderState &state) {
    lv_layer_t layer;
    lv_canvas_init_layer(canvas_, &layer);

    // Sclera (white circle)
    lv_draw_rect_dsc_t sclera_dsc;
    lv_draw_rect_dsc_init(&sclera_dsc);
    sclera_dsc.bg_color = lv_color_white();
    sclera_dsc.bg_opa = LV_OPA_COVER;
    sclera_dsc.radius = SCLERA_R;

    lv_area_t sclera_area = {
        (lv_coord_t)(cx - SCLERA_R), (lv_coord_t)(cy - SCLERA_R),
        (lv_coord_t)(cx + SCLERA_R), (lv_coord_t)(cy + SCLERA_R)
    };
    lv_draw_rect(&layer, &sclera_dsc, &sclera_area);

    // Iris (colored circle, offset by gaze)
    lv_coord_t iris_offset_x = (lv_coord_t)(state.gazeX * GAZE_MAX_PX);
    lv_coord_t iris_offset_y = (lv_coord_t)(state.gazeY * GAZE_MAX_PX);
    lv_coord_t iris_cx = cx + iris_offset_x;
    lv_coord_t iris_cy = cy + iris_offset_y;

    lv_color_t iris_color = state.isChapado ? lv_color_make(0xCC, 0x33, 0x33)
                                            : lv_color_make(0x4A, 0x90, 0xD9);

    lv_draw_rect_dsc_t iris_dsc;
    lv_draw_rect_dsc_init(&iris_dsc);
    iris_dsc.bg_color = iris_color;
    iris_dsc.bg_opa = LV_OPA_COVER;
    iris_dsc.radius = IRIS_R;

    lv_area_t iris_area = {
        (lv_coord_t)(iris_cx - IRIS_R), (lv_coord_t)(iris_cy - IRIS_R),
        (lv_coord_t)(iris_cx + IRIS_R), (lv_coord_t)(iris_cy + IRIS_R)
    };
    lv_draw_rect(&layer, &iris_dsc, &iris_area);

    // Pupil (black circle, same offset as iris)
    lv_draw_rect_dsc_t pupil_dsc;
    lv_draw_rect_dsc_init(&pupil_dsc);
    pupil_dsc.bg_color = lv_color_black();
    pupil_dsc.bg_opa = LV_OPA_COVER;
    pupil_dsc.radius = PUPIL_R;

    lv_area_t pupil_area = {
        (lv_coord_t)(iris_cx - PUPIL_R), (lv_coord_t)(iris_cy - PUPIL_R),
        (lv_coord_t)(iris_cx + PUPIL_R), (lv_coord_t)(iris_cy + PUPIL_R)
    };
    lv_draw_rect(&layer, &pupil_dsc, &pupil_area);

    // Eyelid (black arc covering top when blinkProgress > 0)
    if (state.blinkProgress > 0.01f) {
        lv_draw_arc_dsc_t lid_dsc;
        lv_draw_arc_dsc_init(&lid_dsc);
        lid_dsc.color = lv_color_black();
        lid_dsc.width = SCLERA_R * 2 + 2;
        lid_dsc.start_angle = 0;
        lid_dsc.end_angle = (lv_coord_t)(180.0f * state.blinkProgress);

        lv_draw_arc(&layer, &lid_dsc, &sclera_area);
    }

    lv_canvas_finish_layer(canvas_, &layer);
}
```

- [ ] **Step 3: Update main_desktop.cpp to use EyeRenderer**

Replace the test label section in `main_desktop.cpp` with:

```cpp
#include "eyes/EyeRenderer.h"

// ... in main(), after LVGL init:

EyeRenderer eyes;
eyes.create(lv_scr_act());

EyeRenderState state = {0.0f, 0.0f, 0.0f, false};
```

In the main loop, before `lv_timer_handler()`:

```cpp
eyes.render(state);
```

- [ ] **Step 4: Verify eyes render on simulator**

Run: `pio run -e simulator`
Expected: 240×240 window with two white circles (sclera), blue circles (iris), black circles (pupil), centered on screen

- [ ] **Step 5: Commit**

```bash
git add firmware/src/eyes/EyeRenderer.h firmware/src/eyes/EyeRenderer.cpp firmware/src/main_desktop.cpp
git commit -m "feat: EyeRenderer draws static eyes on canvas (sclera+iris+pupil)"
```

---

## Task 3: EyeState + Gaze Tracking

**Goal:** Pupils move when arrow keys are pressed (simulated accelerometer). Smooth lerp interpolation.

**Files:**
- Create: `firmware/src/eyes/EyeState.h`
- Create: `firmware/src/eyes/EyeState.cpp`
- Modify: `firmware/src/main_desktop.cpp` (integrate EyeState + keyboard input)

**Interfaces:**
- Consumes: `EyeRenderer` from Task 2 (uses `EyeRenderState`)
- Produces: `EyeState` class with `update(dt)` and `getRenderState()` methods
- Produces: `GestureEvent` enum for input system

- [ ] **Step 1: Create EyeState header**

Create `firmware/src/eyes/EyeState.h`:

```cpp
#pragma once
#include "EyeRenderer.h"
#include <cstdint>

class EyeState {
public:
    void update(uint32_t nowMs);
    EyeRenderState getRenderState() const;

    // External input (called by InputAdapter)
    void setGazeTarget(float x, float y); // -1.0 to 1.0

    // Personality interface (called by Personality module)
    void triggerBlink();
    float getBlinkProgress() const;
    float getGazeX() const;
    float getGazeY() const;
    void setGazeOverride(float x, float y);
    void clearGazeOverride();
    void setDwell(bool active);
    void setSleepy(bool active);
    void setChapado(bool active);

private:
    float gazeX_ = 0.0f;
    float gazeY_ = 0.0f;
    float gazeTargetX_ = 0.0f;
    float gazeTargetY_ = 0.0f;
    float blinkProgress_ = 0.0f;
    bool isChapado_ = false;
    bool sleepyActive_ = false;

    // Blink state
    uint32_t lastBlinkMs_ = 0;
    uint32_t blinkIntervalMs_ = 5000;
    bool blinkOpen_ = true;

    // Lerp factor (lower = smoother/slower)
    static constexpr float LERP_FACTOR = 0.15f;
    static constexpr float BLINK_SPEED = 0.2f; // progress per frame at 30fps
};
```

- [ ] **Step 2: Create EyeState implementation**

Create `firmware/src/eyes/EyeState.cpp`:

```cpp
#include "EyeState.h"
#include <cmath>
#include <cstdlib>

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void EyeState::setGazeTarget(float x, float y) {
    gazeTargetX_ = x;
    gazeTargetY_ = y;
}

void EyeState::update(uint32_t nowMs) {
    // Smooth gaze interpolation
    gazeX_ = lerp(gazeX_, gazeTargetX_, LERP_FACTOR);
    gazeY_ = lerp(gazeY_, gazeTargetY_, LERP_FACTOR);

    // Blink logic
    if (blinkOpen_) {
        if (nowMs - lastBlinkMs_ >= blinkIntervalMs_) {
            blinkOpen_ = false;
            lastBlinkMs_ = nowMs;
            blinkIntervalMs_ = 5000 + (rand() % 500); // 5s + jitter
        }
    } else {
        blinkProgress_ += BLINK_SPEED;
        if (blinkProgress_ >= 1.0f) {
            // Fully closed, now open
            blinkProgress_ = 1.0f;
            blinkOpen_ = true;
            lastBlinkMs_ = nowMs;
        }
    }

    if (blinkOpen_ && blinkProgress_ > 0.0f) {
        blinkProgress_ -= BLINK_SPEED;
        if (blinkProgress_ < 0.0f) blinkProgress_ = 0.0f;
    }
}

EyeRenderState EyeState::getRenderState() const {
    return { gazeX_, gazeY_, blinkProgress_, isChapado_ };
}
```

- [ ] **Step 3: Create GestureEvent enum**

Create `firmware/src/input/GestureEngine.h`:

```cpp
#pragma once

enum class GestureEvent {
    NONE,
    SWIPE_UP,
    SWIPE_DOWN,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    TAP,
    SHAKE,
    LONG_PRESS_POWER,
    DOUBLE_CLICK_POWER,
    WRIST_RAISE
};

class GestureEngine {
public:
    int resolveScreen(GestureEvent event, int currentScreen);
};
```

- [ ] **Step 4: Create DesktopInputAdapter**

Create `firmware/src/input/DesktopInputAdapter.h`:

```cpp
#pragma once
#include "GestureEngine.h"
#include <cstdint>

class DesktopInputAdapter {
public:
    // Call each frame to process SDL2 events
    void poll();

    // Get the latest gesture (returns NONE if no gesture)
    GestureEvent getGesture();

    // Get simulated gaze from arrow key state
    float getGazeX() const;
    float getGazeY() const;

    // Simulated shake (Enter key)
    bool shakeDetected() const;

private:
    GestureEvent lastGesture_ = GestureEvent::NONE;
    float gazeX_ = 0.0f;
    float gazeY_ = 0.0f;
    bool shake_ = false;
};
```

- [ ] **Step 5: Create DesktopInputAdapter implementation**

Create `firmware/src/input/DesktopInputAdapter.cpp`:

```cpp
#include "DesktopInputAdapter.h"
#include <SDL2/SDL.h>

void DesktopInputAdapter::poll() {
    lastGesture_ = GestureEvent::NONE;
    shake_ = false;

    // Decay gaze back to center
    gazeX_ *= 0.9f;
    gazeY_ *= 0.9f;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_UP:    gazeY_ = -1.0f; lastGesture_ = GestureEvent::SWIPE_UP; break;
                case SDLK_DOWN:  gazeY_ =  1.0f; lastGesture_ = GestureEvent::SWIPE_DOWN; break;
                case SDLK_LEFT:  gazeX_ = -1.0f; lastGesture_ = GestureEvent::SWIPE_LEFT; break;
                case SDLK_RIGHT: gazeX_ =  1.0f; lastGesture_ = GestureEvent::SWIPE_RIGHT; break;
                case SDLK_SPACE: lastGesture_ = GestureEvent::TAP; break;
                case SDLK_RETURN: shake_ = true; lastGesture_ = GestureEvent::SHAKE; break;
                case SDLK_BACKSPACE: lastGesture_ = GestureEvent::LONG_PRESS_POWER; break;
                default: break;
            }
        }
    }
}

GestureEvent DesktopInputAdapter::getGesture() { return lastGesture_; }
float DesktopInputAdapter::getGazeX() const { return gazeX_; }
float DesktopInputAdapter::getGazeY() const { return gazeY_; }
bool DesktopInputAdapter::shakeDetected() const { return shake_; }
```

- [ ] **Step 6: Update main_desktop.cpp to integrate EyeState + input**

Add includes and variables:

```cpp
#include "eyes/EyeState.h"
#include "input/DesktopInputAdapter.h"

// ... in main():
EyeState eyeState;
DesktopInputAdapter input;
```

In the main loop:

```cpp
input.poll();
eyeState.setGazeTarget(input.getGazeX(), input.getGazeY());
eyeState.update(SDL_GetTicks());
eyes.render(eyeState.getRenderState());
```

- [ ] **Step 7: Verify gaze tracking works**

Run: `pio run -e simulator`
Expected: Eyes rendered. Press arrow keys → pupils move smoothly in that direction. Release → pupils drift back to center.

- [ ] **Step 8: Commit**

```bash
git add firmware/src/eyes/EyeState.h firmware/src/eyes/EyeState.cpp firmware/src/input/GestureEngine.h firmware/src/input/DesktopInputAdapter.h firmware/src/input/DesktopInputAdapter.cpp firmware/src/main_desktop.cpp
git commit -m "feat: EyeState with gaze tracking via keyboard input"
```

---

## Task 4: Personality Behaviors

**Goal:** Eyes blink autonomously, perform saccades, and enter dwell mode when still.

**Files:**
- Create: `firmware/src/eyes/Personality.h`
- Create: `firmware/src/eyes/Personality.cpp`
- Modify: `firmware/src/eyes/EyeState.cpp` (integrate Personality)

**Interfaces:**
- Consumes: `EyeState` (modifies blink, gaze offset)
- Produces: `Personality` class with `update(state, dt)` method

- [ ] **Step 1: Create Personality header**

Create `firmware/src/eyes/Personality.h`:

```cpp
#pragma once
#include <cstdint>

class EyeState; // forward declare

class Personality {
public:
    void update(EyeState &state, uint32_t nowMs);

private:
    // Blink
    uint32_t lastBlinkMs_ = 0;
    uint32_t blinkIntervalMs_ = 5000;
    bool blinkTriggered_ = false;

    // Saccade
    uint32_t lastSaccadeMs_ = 0;
    uint32_t saccadeIntervalMs_ = 4000;
    float saccadeTargetX_ = 0.0f;
    float saccadeTargetY_ = 0.0f;
    bool saccadeActive_ = false;
    uint32_t saccadeStartMs_ = 0;

    // Dwell
    uint32_t lastMovementMs_ = 0;
    bool dwellActive_ = false;

    // Sleepy
    uint32_t idleStartMs_ = 0;
    bool sleepyActive_ = false;

    // Chapado
    bool chapadoTriggered_ = false;
    uint32_t shakeCount_ = 0;
    uint32_t lastShakeMs_ = 0;

    static constexpr uint32_t DWELL_THRESHOLD_MS = 3000;
    static constexpr uint32_t SLEEPY_THRESHOLD_MS = 30000;
    static constexpr uint32_t SHAKE_WINDOW_MS = 2000;
    static constexpr uint32_t SHAKE_COUNT_TRIGGER = 3;
};
```

- [ ] **Step 2: Create Personality implementation**

Create `firmware/src/eyes/Personality.cpp`:

```cpp
#include "Personality.h"
#include "EyeState.h"
#include <cstdlib>
#include <cmath>

void Personality::update(EyeState &state, uint32_t nowMs) {
    // --- Blink ---
    if (!blinkTriggered_ && (nowMs - lastBlinkMs_ >= blinkIntervalMs_)) {
        state.triggerBlink();
        blinkTriggered_ = true;
        lastBlinkMs_ = nowMs;
        blinkIntervalMs_ = 5000 + (rand() % 500);
    }
    if (blinkTriggered_ && state.getBlinkProgress() < 0.01f) {
        blinkTriggered_ = false; // blink finished
    }

    // --- Saccade ---
    if (!saccadeActive_ && (nowMs - lastSaccadeMs_ >= saccadeIntervalMs_)) {
        saccadeTargetX_ = ((float)(rand() % 100) / 100.0f) * 0.6f - 0.3f; // -0.3 to 0.3
        saccadeTargetY_ = ((float)(rand() % 100) / 100.0f) * 0.4f - 0.2f;
        saccadeActive_ = true;
        saccadeStartMs_ = nowMs;
        lastSaccadeMs_ = nowMs;
        saccadeIntervalMs_ = 3000 + (rand() % 5000);
    }
    if (saccadeActive_) {
        uint32_t elapsed = nowMs - saccadeStartMs_;
        if (elapsed < 150) {
            // Jump to saccade target
            state.setGazeOverride(saccadeTargetX_, saccadeTargetY_);
        } else if (elapsed < 400) {
            // Return to normal gaze
            state.clearGazeOverride();
        } else {
            saccadeActive_ = false;
        }
    }

    // --- Dwell (when gaze hasn't moved) ---
    float gx = state.getGazeX();
    float gy = state.getGazeY();
    if (fabsf(gx) < 0.05f && fabsf(gy) < 0.05f) {
        if (!dwellActive_ && (nowMs - lastMovementMs_ > DWELL_THRESHOLD_MS)) {
            dwellActive_ = true;
            state.setDwell(true);
        }
    } else {
        lastMovementMs_ = nowMs;
        if (dwellActive_) {
            dwellActive_ = false;
            state.setDwell(false);
        }
    }

    // --- Sleepy ---
    if (nowMs - lastMovementMs_ > SLEEPY_THRESHOLD_MS) {
        if (!sleepyActive_) {
            sleepyActive_ = true;
            state.setSleepy(true);
        }
    } else {
        if (sleepyActive_) {
            sleepyActive_ = false;
            state.setSleepy(false);
        }
    }

    // --- Chapado shake detection ---
    // (called externally when shake gesture detected)
}

void Personality::onShake(uint32_t nowMs) {
    if (nowMs - lastShakeMs_ > SHAKE_WINDOW_MS) {
        shakeCount_ = 1;
    } else {
        shakeCount_++;
    }
    lastShakeMs_ = nowMs;

    if (shakeCount_ >= SHAKE_COUNT_TRIGGER) {
        chapadoTriggered_ = !chapadoTriggered_; // toggle
        shakeCount_ = 0;
    }
}

bool Personality::isChapado() const { return chapadoTriggered_; }
```

- [ ] **Step 3: Add state methods to EyeState**

Add to `EyeState.h`:

```cpp
void triggerBlink();
float getBlinkProgress() const;
float getGazeX() const;
float getGazeY() const;
void setGazeOverride(float x, float y);
void clearGazeOverride();
void setDwell(bool active);
void setSleepy(bool active);
void setChapado(bool active);
```

Add to `EyeState.cpp`:

```cpp
void EyeState::triggerBlink() { blinkOpen_ = false; }
float EyeState::getBlinkProgress() const { return blinkProgress_; }
float EyeState::getGazeX() const { return gazeX_; }
float EyeState::getGazeY() const { return gazeY_; }

void EyeState::setGazeOverride(float x, float y) {
    gazeX_ = x;
    gazeY_ = y;
}
void EyeState::clearGazeOverride() { /* lerp will handle return */ }

void EyeState::setDwell(bool active) {
    // Dwell: slightly reduce gaze movement range
    // (visual effect handled by render)
}

void EyeState::setSleepy(bool active) {
    sleepyActive_ = active;
    // Sleepy: reduce blink speed, lower eyelid baseline
}

void EyeState::setChapado(bool active) {
    isChapado_ = active;
}
```

- [ ] **Step 4: Integrate Personality in main_desktop.cpp**

```cpp
#include "eyes/Personality.h"

// ... in main():
Personality personality;

// In main loop:
personality.update(eyeState, SDL_GetTicks());
if (input.shakeDetected()) {
    personality.onShake(SDL_GetTicks());
}
eyeState.setChapado(personality.isChapado());
```

- [ ] **Step 5: Verify personality behaviors**

Run: `pio run -e simulator`
Expected:
- Eyes blink automatically every ~5s
- Pupils occasionally jump to random positions (saccades)
- After 3s of no input, eyes "dwell" (fixate)
- After 30s idle, eyes get sleepy (lower eyelids)
- Press Enter 3x quickly → eyes turn red (chapado mode)

- [ ] **Step 6: Commit**

```bash
git add firmware/src/eyes/Personality.h firmware/src/eyes/Personality.cpp firmware/src/eyes/EyeState.h firmware/src/eyes/EyeState.cpp firmware/src/main_desktop.cpp
git commit -m "feat: personality behaviors (blink, saccade, dwell, sleepy, chapado)"
```

---

## Task 5: Time Display

**Goal:** Show current time and date as LVGL labels. Updates automatically.

**Files:**
- Create: `firmware/src/watchface/WatchFace.h`
- Create: `firmware/src/watchface/WatchFace.cpp`
- Create: `firmware/src/watchface/TimeDisplay.h`
- Create: `firmware/src/watchface/TimeDisplay.cpp`
- Create: `firmware/src/watchface/DateDisplay.h`
- Create: `firmware/src/watchface/DateDisplay.cpp`
- Modify: `firmware/src/main_desktop.cpp` (integrate WatchFace)

**Interfaces:**
- Consumes: `lv_scr_act()` for parent object
- Produces: `WatchFace` class that manages all UI elements
- Produces: `TimeDisplay` and `DateDisplay` as child components

- [ ] **Step 1: Create TimeDisplay header + impl**

Create `firmware/src/watchface/TimeDisplay.h`:

```cpp
#pragma once
#include "lvgl.h"

class TimeDisplay {
public:
    void create(lv_obj_t *parent);
    void update(); // call every minute
    lv_obj_t *label() const { return label_; }

private:
    lv_obj_t *label_ = nullptr;
    int lastMinute_ = -1;
};
```

Create `firmware/src/watchface/TimeDisplay.cpp`:

```cpp
#include "TimeDisplay.h"
#include <cstdio>
#include <ctime>

void TimeDisplay::create(lv_obj_t *parent) {
    label_ = lv_label_create(parent);
    lv_obj_set_style_text_font(label_, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_set_style_text_opa(label_, LV_OPA_80, 0);
    lv_obj_align(label_, LV_ALIGN_TOP_MID, 0, 8);
    update();
}

void TimeDisplay::update() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t->tm_min != lastMinute_) {
        lastMinute_ = t->tm_min;
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);
        lv_label_set_text(label_, buf);
    }
}
```

- [ ] **Step 2: Create DateDisplay header + impl**

Create `firmware/src/watchface/DateDisplay.h`:

```cpp
#pragma once
#include "lvgl.h"

class DateDisplay {
public:
    void create(lv_obj_t *parent);
    void update(); // call every hour
};
```

Create `firmware/src/watchface/DateDisplay.cpp`:

```cpp
#include "DateDisplay.h"
#include <cstdio>
#include <ctime>

static const char *DAYS[] = {"dom", "seg", "ter", "qua", "qui", "sex", "sab"};
static const char *MONTHS[] = {"jan","fev","mar","abr","mai","jun",
                                "jul","ago","set","out","nov","dez"};

void DateDisplay::create(lv_obj_t *parent) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_50, 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -8);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char buf[32];
    snprintf(buf, sizeof(buf), "%s %d %s", DAYS[t->tm_wday], t->tm_mday, MONTHS[t->tm_mon]);
    lv_label_set_text(label, buf);
}
```

- [ ] **Step 3: Create WatchFace layout manager**

Create `firmware/src/watchface/WatchFace.h`:

```cpp
#pragma once
#include "lvgl.h"
#include "TimeDisplay.h"
#include "DateDisplay.h"
#include "../eyes/EyeRenderer.h"

class WatchFace {
public:
    void create(lv_obj_t *parent);
    void update(); // call periodically

    TimeDisplay timeDisplay;
    DateDisplay dateDisplay;
    EyeRenderer eyes;
};
```

Create `firmware/src/watchface/WatchFace.cpp`:

```cpp
#include "WatchFace.h"

void WatchFace::create(lv_obj_t *parent) {
    timeDisplay.create(parent);
    dateDisplay.create(parent);
    eyes.create(parent);
}

void WatchFace::update() {
    timeDisplay.update();
}
```

- [ ] **Step 4: Update main_desktop.cpp to use WatchFace**

Replace individual EyeRenderer/EyeState usage with WatchFace:

```cpp
#include "watchface/WatchFace.h"
#include "eyes/EyeState.h"
#include "input/DesktopInputAdapter.h"

// In main():
WatchFace watchFace;
watchFace.create(lv_scr_act());
EyeState eyeState;
DesktopInputAdapter input;

// In main loop:
input.poll();
eyeState.setGazeTarget(input.getGazeX(), input.getGazeY());
eyeState.update(SDL_GetTicks());
watchFace.eyes.render(eyeState.getRenderState());
watchFace.update();
```

- [ ] **Step 5: Verify time and date display**

Run: `pio run -e simulator`
Expected: Time "HH:MM" at top center (white, 80% alpha), date "seg 17 ago" at bottom center (white, 50% alpha), eyes in center. Time updates when minute changes.

- [ ] **Step 6: Commit**

```bash
git add firmware/src/watchface/ firmware/src/main_desktop.cpp
git commit -m "feat: WatchFace with time display + date display + eyes"
```

---

## Task 6: Gesture Navigation (Shells)

**Goal:** Swipe to navigate between watch face and shell screens. Fade transitions.

**Files:**
- Create: `firmware/src/input/GestureEngine.cpp`
- Modify: `firmware/src/watchface/WatchFace.h/.cpp` (add screen management)
- Modify: `firmware/src/main_desktop.cpp` (handle gestures)

**Interfaces:**
- Consumes: `GestureEngine` from Task 3
- Produces: Screen transitions with fade animations

- [ ] **Step 1: Create GestureEngine implementation**

Create `firmware/src/input/GestureEngine.cpp`:

```cpp
#include "GestureEngine.h"

struct GestureMapping {
    GestureEvent trigger;
    int targetScreen; // screen index
};

// Default gesture map (could be loaded from gestures.json later)
static const GestureMapping DEFAULT_MAP[] = {
    { GestureEvent::SWIPE_UP,    1 }, // notifications
    { GestureEvent::SWIPE_RIGHT, 2 }, // apps
    { GestureEvent::SWIPE_DOWN,  3 }, // settings
    { GestureEvent::TAP,         0 }, // back to watch face
};

static constexpr int MAP_SIZE = sizeof(DEFAULT_MAP) / sizeof(DEFAULT_MAP[0]);

int GestureEngine::resolveScreen(GestureEvent event, int currentScreen) {
    if (event == GestureEvent::TAP && currentScreen != 0) {
        return 0; // tap always goes back to watch face
    }
    for (int i = 0; i < MAP_SIZE; i++) {
        if (DEFAULT_MAP[i].trigger == event) {
            return DEFAULT_MAP[i].targetScreen;
        }
    }
    return currentScreen; // no change
}
```

- [ ] **Step 2: Add screen management to WatchFace**

Add to `WatchFace.h`:

```cpp
enum class Screen { WATCH_FACE, NOTIFICATIONS, APPS, SETTINGS };

class WatchFace {
public:
    void create(lv_obj_t *parent);
    void update();
    void transitionTo(Screen screen);

    Screen currentScreen() const;

    // ... existing members
private:
    Screen current_ = Screen::WATCH_FACE;
    lv_obj_t *notifLabel_ = nullptr;
    lv_obj_t *appsLabel_ = nullptr;
    lv_obj_t *settingsLabel_ = nullptr;
};
```

Add to `WatchFace.cpp`:

```cpp
void WatchFace::create(lv_obj_t *parent) {
    // Create all screens (hidden by default)
    timeDisplay.create(parent);
    dateDisplay.create(parent);
    eyes.create(parent);

    // Notification shell
    notifLabel_ = lv_label_create(parent);
    lv_label_set_text(notifLabel_, "Notifications\n(swipe down to go back)");
    lv_obj_set_style_text_color(notifLabel_, lv_color_white(), 0);
    lv_obj_align(notifLabel_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(notifLabel_, LV_OBJ_FLAG_HIDDEN);

    // Apps shell
    appsLabel_ = lv_label_create(parent);
    lv_label_set_text(appsLabel_, "Apps\n(swipe left to go back)");
    lv_obj_set_style_text_color(appsLabel_, lv_color_white(), 0);
    lv_obj_align(appsLabel_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(appsLabel_, LV_OBJ_FLAG_HIDDEN);

    // Settings shell
    settingsLabel_ = lv_label_create(parent);
    lv_label_set_text(settingsLabel_, "Settings\n(swipe up to go back)");
    lv_obj_set_style_text_color(settingsLabel_, lv_color_white(), 0);
    lv_obj_align(settingsLabel_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(settingsLabel_, LV_OBJ_FLAG_HIDDEN);
}

void WatchFace::transitionTo(Screen screen) {
    // Hide all
    lv_obj_add_flag(notifLabel_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(appsLabel_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(settingsLabel_, LV_OBJ_FLAG_HIDDEN);

    current_ = screen;

    switch (screen) {
        case Screen::WATCH_FACE:
            lv_obj_clear_flag(timeDisplay.label(), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(dateDisplay.label(), LV_OBJ_FLAG_HIDDEN);
            // eyes always visible
            break;
        case Screen::NOTIFICATIONS:
            lv_obj_clear_flag(notifLabel_, LV_OBJ_FLAG_HIDDEN);
            break;
        case Screen::APPS:
            lv_obj_clear_flag(appsLabel_, LV_OBJ_FLAG_HIDDEN);
            break;
        case Screen::SETTINGS:
            lv_obj_clear_flag(settingsLabel_, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}

Screen WatchFace::currentScreen() const { return current_; }
```

- [ ] **Step 3: Integrate gesture navigation in main_desktop.cpp**

```cpp
#include "input/GestureEngine.h"

// In main():
GestureEngine gestureEngine;

// In main loop, after input.poll():
if (input.getGesture() != GestureEvent::NONE) {
    int target = gestureEngine.resolveScreen(input.getGesture(), (int)watchFace.currentScreen());
    watchFace.transitionTo((Screen)target);
}
```

- [ ] **Step 4: Verify navigation works**

Run: `pio run -e simulator`
Expected:
- Watch face shown by default (eyes + time + date)
- Swipe up (↑) → "Notifications" text shown
- Swipe right (→) → "Apps" text shown
- Swipe down (↓) → "Settings" text shown
- Tap (Space) → back to watch face
- Eyes continue animating on watch face screen

- [ ] **Step 5: Commit**

```bash
git add firmware/src/input/GestureEngine.cpp firmware/src/watchface/ firmware/src/main_desktop.cpp
git commit -m "feat: gesture navigation between watch face and shell screens"
```

---

## Task 7: Status Icons

**Goal:** Show battery, Wi-Fi, BLE status as small icons next to time.

**Files:**
- Create: `firmware/src/watchface/StatusIcons.h`
- Create: `firmware/src/watchface/StatusIcons.cpp`
- Modify: `firmware/src/watchface/WatchFace.h/.cpp` (integrate StatusIcons)

**Interfaces:**
- Consumes: `lv_scr_act()` parent
- Produces: `StatusIcons` class with `update()` method

- [ ] **Step 1: Create StatusIcons header + impl**

Create `firmware/src/watchface/StatusIcons.h`:

```cpp
#pragma once
#include "lvgl.h"

class StatusIcons {
public:
    void create(lv_obj_t *parent);
    void update();

    void setBatteryPercent(int percent);  // 0-100
    void setWifiConnected(bool connected);
    void setBleConnected(bool connected);
};
```

Create `firmware/src/watchface/StatusIcons.cpp`:

```cpp
#include "StatusIcons.h"
#include <cstdio>

static lv_obj_t *batteryLabel_ = nullptr;
static lv_obj_t *wifiLabel_ = nullptr;
static lv_obj_t *bleLabel_ = nullptr;

void StatusIcons::create(lv_obj_t *parent) {
    // Battery icon (top-right area)
    batteryLabel_ = lv_label_create(parent);
    lv_obj_set_style_text_font(batteryLabel_, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(batteryLabel_, lv_color_white(), 0);
    lv_obj_set_style_text_opa(batteryLabel_, LV_OPA_40, 0);
    lv_obj_align(batteryLabel_, LV_ALIGN_TOP_RIGHT, -8, 8);
    lv_label_set_text(batteryLabel_, "100%");

    // Wi-Fi icon
    wifiLabel_ = lv_label_create(parent);
    lv_obj_set_style_text_font(wifiLabel_, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(wifiLabel_, lv_color_white(), 0);
    lv_obj_set_style_text_opa(wifiLabel_, LV_OPA_40, 0);
    lv_obj_align(wifiLabel_, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_label_set_text(wifiLabel_, "wifi:off");

    // BLE icon
    bleLabel_ = lv_label_create(parent);
    lv_obj_set_style_text_font(bleLabel_, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(bleLabel_, lv_color_white(), 0);
    lv_obj_set_style_text_opa(bleLabel_, LV_OPA_40, 0);
    lv_obj_align(bleLabel_, LV_ALIGN_TOP_LEFT, 8, 20);
    lv_label_set_text(bleLabel_, "ble:off");
}

void StatusIcons::setBatteryPercent(int percent) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    lv_label_set_text(batteryLabel_, buf);
}

void StatusIcons::setWifiConnected(bool connected) {
    lv_label_set_text(wifiLabel_, connected ? "wifi:on" : "wifi:off");
}

void StatusIcons::setBleConnected(bool connected) {
    lv_label_set_text(bleLabel_, connected ? "ble:on" : "ble:off");
}
```

- [ ] **Step 2: Integrate in WatchFace**

Add to `WatchFace.h`:

```cpp
#include "StatusIcons.h"

class WatchFace {
    // ... existing
    StatusIcons statusIcons;
};
```

In `WatchFace.cpp` `create()`:

```cpp
statusIcons.create(parent);
// Mock values for simulator
statusIcons.setBatteryPercent(87);
statusIcons.setWifiConnected(true);
statusIcons.setBleConnected(false);
```

- [ ] **Step 3: Verify status icons**

Run: `pio run -e simulator`
Expected: "87%" top-right, "wifi:on" top-left, "ble:off" below wifi — all small and subtle

- [ ] **Step 4: Commit**

```bash
git add firmware/src/watchface/StatusIcons.h firmware/src/watchface/StatusIcons.cpp firmware/src/watchface/WatchFace.h firmware/src/watchface/WatchFace.cpp
git commit -m "feat: status icons (battery, wifi, ble)"
```

---

## Summary

| Task | Deliverable | Est. Time |
|------|-------------|-----------|
| 1 | Simulator environment (empty window) | 15 min |
| 2 | EyeRenderer (static eyes) | 20 min |
| 3 | EyeState + gaze tracking | 20 min |
| 4 | Personality (blink, saccade, dwell, sleepy, chapado) | 25 min |
| 5 | Time + date display | 15 min |
| 6 | Gesture navigation (shell screens) | 20 min |
| 7 | Status icons | 10 min |
| **Total** | **Full watch face on PC simulator** | **~2h** |

**After Task 7:** You have a complete watch face running on PC with animated eyes, time, date, status icons, and gesture navigation. The same code compiles for ESP32-S3 when hardware arrives (Task 8+ = hardware port, deferred).
