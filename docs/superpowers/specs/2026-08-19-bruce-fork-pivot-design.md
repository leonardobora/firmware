# Design: Pivot to Bruce Firmware Fork

**Date:** 2026-08-19
**Status:** Draft
**Author:** Leonardo Bora + AI assistant

---

## Context

The b0r4-watch project started as a custom firmware for the LILYGO T-Watch S3 (ESP32-S3), building a smartwatch from scratch with Arduino + PlatformIO + LVGL. After Phase 0 (avatar on simulator, voice assistant server), we discovered [Bruce firmware](https://bruce.computer/) — a mature, community-driven ESP32 firmware with 6.5k stars that already supports the T-Watch S3 and includes extensive hardware modules (WiFi attacks, SubGHz, NFC, RFID, IR, GPS, LoRa, BLE, FM radio, JS interpreter).

**The problem:** Building hardware drivers, radio modules, and basic firmware infrastructure from scratch takes months. Bruce already has all of this working.

**The decision:** Fork Bruce firmware and add smartwatch features on top, creating a hybrid device that is both a hacker tool AND a personal assistant.

---

## Goals

1. Get a working smartwatch on real T-Watch S3 hardware in ~2-3 weeks
2. Have all Bruce security tools (WiFi, IR, RF, GPS, etc.) available immediately
3. Add watchface, voice assistant, music player, notifications, and gesture control
4. Keep the Python server independent (MIT license, reusable)
5. Contribute back to the Bruce community where possible

---

## Architecture

### Repository Structure

```
b0r4-watch/  (fork of BruceDevices/firmware)
├── src/
│   ├── core/           # FROM BRUCE: WiFi, BLE, display, input, themes, filesystem
│   ├── modules/        # FROM BRUCE: CC1101, NRF24, IR, GPS, NFC, RFID, LoRa, FM
│   ├── apps/           # FROM BRUCE: evil portal, deauth, wardriving, etc.
│   ├── watch/          # NEW: b0r4-watch exclusive modules
│   │   ├── watchface/  # Clock face + animated avatar
│   │   ├── assistant/  # Voice assistant WebSocket client
│   │   ├── music/      # Music player (YouTube Music via server)
│   │   ├── gestures/   # Gesture engine (BMA423 → actions)
│   │   ├── ancs/       # Apple Notification Center Service
│   │   ├── gps_ble/    # GPS via BLE from iPhone
│   │   └── energy/     # Deep sleep + power management
│   └── main.cpp        # Entry point (modified from Bruce)
├── server/             # INDEPENDENT: Python FastAPI (MIT license)
├── themes/             # FROM BRUCE: customizable themes
├── bruce.conf          # Bruce configuration
├── platformio.ini      # Bruce base + b0r4-watch extra libs
└── docs/               # OUR docs: ADRs, glossary, input system
```

### UI Layout

```
                          SWIPE UP
                            │
                    ┌───────────────┐
                    │  MUSIC + GPS  │
                    │  player +     │
                    │  coordinates  │
                    └───────────────┘
                            │
SWIPE LEFT        SWIPE RIGHT │
    │                  │      │
┌───────────┐    ┌───────────┐    ┌───────────┐
│ ASSISTANT │◄──►│ WATCHFACE │◄──►│  HACKER   │
│ de voz    │    │ hora +    │    │  tools    │
│ (server)  │    │ avatar    │    │  (Bruce)  │
└───────────┘    └───────────┘    └───────────┘
                            │
                    ┌───────────────┐
                    │ NOTIFICATIONS │
                    │ WhatsApp      │
                    │ (filtered)    │
                    └───────────────┘
                            │
                          SWIPE DOWN
```

**Navigation rules:**
- Swipe left/right: navigate between 3 main screens (Assistant ↔ Watchface ↔ Hacker)
- Swipe up from watchface: Music + GPS submenu
- Swipe down from watchface: Notifications submenu
- Tap on watchface: Opens Bruce launcher (app drawer)
- Power button: Returns to watchface (home)
- Power hold 3s: Deep sleep

---

## Components

### 1. Watchface (NEW)

**Purpose:** Default screen showing time, date, avatar, and status.

**Components:**
- Digital clock (large font, HH:MM)
- Date display (localized pt-BR)
- Animated avatar (center, GIF/sprite-based — NOT ASCII)
- Status bar: battery, WiFi, BLE, LoRa icons
- Tap target: opens Bruce launcher

**Implementation:**
- LVGL widgets (label, image, arc for battery)
- Avatar: sprite sheet or GIF decoder (TJpgDec or similar)
- Timer-based updates (1s for clock, 30fps for avatar)

### 2. Animated Avatar (NEW)

**Purpose:** Dynamic character on the watchface (replaces ASCII concept).

**Format options (to be decided during implementation):**
- **GIF player:** Pre-rendered animations, lightweight, rich visual
- **Sprite sheet:** Frame-by-frame animation via LVGL image sequences
- **Lottie:** Vector animations (heavier, needs parser)
- **Procedural canvas:** Drawn in real-time (most flexible, most work)

**Recommended:** GIF/sprite sheet for v1. Simple, proven, lightweight.

**Behaviors:**
- Idle: subtle breathing/blinking animation
- Wake: alert animation
- Music playing: dancing/bopping animation
- Notification received: surprised animation
- Low battery: sleepy animation

### 3. Voice Assistant (ADAPTED from b0r4-watch)

**Purpose:** ASR → LLM → TTS pipeline via WebSocket to Python server.

**Firmware side (NEW):**
- WebSocket client connecting to server's `/ws/audio`
- Captures audio from T-Watch microphone (SPM1423 or MEMS)
- Streams PCM 16kHz to server
- Receives text responses and audio back
- UI: conversation view with transcription + response text

**Server side (EXISTS):**
- FastAPI WebSocket endpoint `/ws/audio`
- ASR (Whisper/mock), LLM (GPT/mock), TTS (Edge/OpenAI/mock)
- Tools: music_play, music_next, music_pause, lights_toggle, system_time

**Change from original:** The server was designed for the b0r4-watch firmware. Now it serves the Bruce fork. Protocol stays the same.

### 4. Music Player (NEW)

**Purpose:** Control YouTube Music playback from the watch.

**Firmware side (NEW):**
- WebSocket client to server's `/ws/music`
- UI: album art (if available), track name, play/pause/next/prev buttons
- Media key emulation via server

**Server side (NEEDS NEW ENDPOINT):**
- `/ws/music` — bidirectional: server pushes track info, watch sends commands
- Uses existing `ytmusicapi` integration
- Falls back to system media keys if YouTube Music not available

### 5. Gesture Engine (ADAPTED from b0r4-watch)

**Purpose:** Translate accelerometer/button/touch events into actions.

**Triggers (from BMA423 accelerometer):**
| Trigger | Action | Context |
|---|---|---|
| Wrist lift | Wake display | Any |
| Shake 2x left | Open assistant | Watchface |
| Shake 2x right | Open hacker tools | Watchface |
| Double-click Power | Play/pause music | Any |
| Tap on avatar | Open Bruce launcher | Watchface |
| Hold Power 3s | Deep sleep | Any |

**Implementation:**
- `gestures.json` config file (same as b0r4-watch)
- GestureEngine reads JSON, maps triggers to actions
- Configurable without recompiling firmware

### 6. ANCS - Notifications (NEW)

**Purpose:** Show iPhone notifications on the watch (filtered).

**Protocol:** Apple Notification Center Service (BLE)
- Already documented in ADR 0006
- Firmware subscribes to ANCS characteristic
- Filters by contact/group (configurable in `bruce.conf` or separate config)
- Shows: app name, title, message preview
- Tap: expand full message
- Swipe right: dismiss

**Filtering:**
- Whitelist of contacts/groups (WhatsApp, iMessage, etc.)
- Configurable via WebUI or config file
- Default: show all notifications (no filter)

### 7. GPS via BLE (NEW)

**Purpose:** Get location from iPhone via BLE.

**How it works:**
- iOS exposes location via GATT Location and Navigation Profile
- Firmware subscribes to location characteristic
- If iPhone not connected: server falls back to IP-based geolocation
- Coordinates displayed on Music + GPS screen

**Fallback chain:**
1. iPhone BLE GPS (most accurate)
2. Server IP geolocation (less accurate)
3. "Sem GPS" (no location)

### 8. Energy Management (ADAPTED from ADR 0005)

**Power modes:**

| Mode | Display | BLE | WiFi | LoRa | Mic | Expected battery |
|---|---|---|---|---|---|---|
| Watch (default) | On, dim after 10s | Always-on | Off | Off | Off | Days |
| Assistant | On | On | On | Off | On | Minutes |
| Hacker | On | On | Per tool | Per tool | Off | Minutes |
| Deep sleep | Off | Off | Off | Off | Off | Weeks |

**Deep sleep wake sources:**
- Touch screen tap
- BMA423 accelerometer (wrist lift, shake)
- RTC alarm
- Power button press

---

## Server Changes

The Python server (`server/`) stays independent but needs new endpoints:

| Endpoint | Method | Purpose |
|---|---|---|
| `/ws/audio` | WebSocket | Voice assistant pipeline (EXISTS) |
| `/ws/music` | WebSocket | Music player control (NEW) |
| `/api/notifications` | POST | Receive notification history (NEW) |
| `/api/gps` | GET | Last known location (NEW) |
| `/health` | GET | Server status (EXISTS) |

**No changes to existing ASR/LLM/TTS pipeline.** The WebSocket protocol stays the same.

---

## What We Get From Bruce (Zero Effort)

| Category | Features |
|---|---|
| WiFi | Evil Portal, Deauth, Scan, Wardriving, EAPOL capture, Probe capture |
| BLE | Scan, Spoofing, spam, tools |
| SubGHz | CC1101 transmit/receive, RAW, protocols |
| NRF24 | Wireless attack tools |
| NFC | Read/write/spoof |
| RFID | Read/write/clone (LF/HF) |
| IR | Transmit/receive, universal remote |
| GPS | NMEA parsing, waypoint, geo-fence |
| LoRa | Transmit/receive, mesh |
| FM | Radio transmit/receive |
| JS | JavaScript interpreter for scripts |
| UI | Launcher, themes, WebUI, serial control |
| Storage | SD card, SPIFFS, LittleFS |
| Config | bruce.conf, theme customization |

---

## What We Build New

| Module | Effort | Complexity | Dependencies |
|---|---|---|---|
| Watchface UI | 2-3 days | Medium | LVGL, Bruce display drivers |
| Animated avatar (GIF/sprite) | 2-3 days | Medium | LVGL image, TJpgDec |
| Gesture engine | 1-2 days | Low | BMA423 (Bruce has driver) |
| Assistant WS client | 2-3 days | Medium | WebSocket lib, mic driver |
| Music player UI + WS | 1-2 days | Low | WebSocket lib |
| ANCS notifications | 2-3 days | Medium | BLE (Bruce has stack) |
| GPS via BLE | 1-2 days | Medium | BLE GATT client |
| Energy management | 1 day | Low | ESP32 sleep APIs |

**Total estimated effort:** ~2-3 weeks with hardware in hand.

---

## What We Keep From b0r4-watch

| Asset | Status | Reuse |
|---|---|---|
| Server Python (FastAPI) | Working | Direct reuse, add endpoints |
| ASR/LLM/TTS pipeline | Working | Direct reuse |
| Music/lights tools | Stubs ready | Direct reuse |
| ADRs (0001-0008) | Complete | Reference docs |
| Glossary (CONTEXT.md) | Complete | Domain language |
| Input system design | Documented | Adapted for Bruce |

---

## Risks & Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Bruce upstream breaks our code | High | Keep `src/watch/` isolated; regular merge from upstream |
| AGPL license | Low (user doesn't mind) | Server stays MIT; firmware is AGPL |
| T-Watch S3 hardware not yet arrived | Medium | Develop on simulator first; Bruce already has working T-Watch support |
| Battery life with always-on BLE | Medium | Profile on real hardware; optimize sleep modes |
| Voice assistant latency over WiFi | Medium | Edge TTS for speed; mock mode for testing |
| Avatar animation performance | Low | GIF/sprite is proven on ESP32-S3; fallback to simpler animation |

---

## Migration Plan

### Phase 1: Setup (Week 1)
1. Fork Bruce firmware on GitHub
2. Set up development environment
3. Flash Bruce to T-Watch S3 (when hardware arrives)
4. Test all Bruce features on real hardware
5. Identify integration points for b0r4-watch modules

### Phase 2: Watchface (Week 1-2)
1. Create `src/watch/watchface/` module
2. Implement clock display + status bar
3. Add animated avatar (GIF/sprite)
4. Set as default screen on boot
5. Add swipe navigation hooks

### Phase 3: Core Features (Week 2)
1. Gesture engine (`src/watch/gestures/`)
2. Assistant WebSocket client (`src/watch/assistant/`)
3. Music player (`src/watch/music/`)
4. ANCS notifications (`src/watch/ancs/`)
5. GPS via BLE (`src/watch/gps_ble/`)

### Phase 4: Server Updates (Week 2-3)
1. Add `/ws/music` endpoint
2. Add `/api/notifications` endpoint
3. Add `/api/gps` endpoint
4. Test full pipeline with firmware

### Phase 5: Polish (Week 3)
1. Energy management tuning
2. UI polish and theme
3. Documentation updates
4. First release tag

---

## Success Criteria

- [ ] Bruce firmware boots on T-Watch S3 with watchface as default
- [ ] Watchface shows time, date, avatar, and status icons
- [ ] Swipe navigation works between all 5 screens
- [ ] Voice assistant transcribes and responds via server
- [ ] Music player controls YouTube Music via server
- [ ] Notifications from iPhone appear on watch (filtered)
- [ ] GPS coordinates shown (from iPhone BLE or server fallback)
- [ ] All Bruce hacker tools accessible from launcher
- [ ] Battery lasts >1 day in watch mode
- [ ] Gesture engine responds to wrist lift, shake, button combos

---

## Repository Strategy

The current `b0r4-watch` repo has custom firmware code (Phase 0 avatar, voice assistant skeleton). The fork introduces a completely different codebase (Bruce).

**Approach:** Create a new repo `b0r4-watch-bruce` as the fork of BruceDevices/firmware. The original `b0r4-watch` repo stays as-is (reference + server code). The server/ directory gets copied into the new repo.

**Migration:**
- New repo: `leonardobora/b0r4-watch-bruce` (fork of BruceDevices/firmware)
- Server code: copied from `b0r4-watch/server/` (independent, MIT)
- Docs: ADRs and glossary copied as reference
- Old repo: archived or kept as "design history"

---

## Open Questions

1. **Avatar format:** GIF vs sprite sheet vs Lottie? Decision deferred to implementation.
2. **Notification filter config:** Where to store contact whitelist? `bruce.conf` or separate file?
3. **GPS BLE protocol:** Which GATT service to use? iOS doesn't expose location directly via BLE — may need a companion app.
4. **Server hosting:** Local PC vs Raspberry Pi vs cloud? Affects latency and availability.
5. **Theme:** Keep Bruce default theme or create custom watch-optimized theme?
