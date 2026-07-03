# pgpemu-esp32 — Firmware Documentation

## Overview

pgpemu-esp32 is an ESP32 firmware that emulates a **Pokémon GO Plus** accessory. The device connects to the Pokémon GO app on Android via Bluetooth Low Energy (BLE), performs catch and Pokéstop actions automatically, and shows its status on an OLED display.

Originally developed for the ESP32-C3, the firmware has been ported to the **Heltec WiFi Kit 32** (classic ESP32, 26 MHz crystal, 128×64 SSD1306 OLED).

---

## Hardware

| Property           | Value                         |
|--------------------|-------------------------------|
| Chip               | ESP32 (Xtensa LX6, dual core) |
| Crystal            | 26 MHz                        |
| Display            | 128×64 SSD1306, I²C           |
| BLE stack          | Bluedroid                     |
| Flash              | 2 MB                          |
| Supply voltage     | 3.3 V / USB 5 V               |

### GPIO assignment

| GPIO | Function              | Direction |
|------|-----------------------|-----------|
| 0    | PRG button            | Input     |
| 4    | OLED SDA (I²C)        | I/O       |
| 15   | OLED SCL (I²C)        | Output    |
| 16   | OLED RST              | Output    |
| 25   | LED (advertising)     | Output    |
| 37   | VBAT sense (ADC1_CH1, 1:2 divider) | Input |

> **Important:** On the classic ESP32, GPIO 6–11 are reserved for the internal SPI flash and must not be used (unlike on the ESP32-C3).

---

## Build & Flash

### Prerequisites

- ESP-IDF v5.4.x
- Docker image: `espressif/idf:v5.4.4`

### Provide secrets

The file `secrets.csv` contains device-specific secrets read out of a real Pokémon GO Plus:

```
key,type,encoding,value
pgpsecret,namespace,,
name,data,string,"DEVICENAME"
mac,data,hex2bin,AABBCCDDEEFF
dkey,data,hex2bin,<16-byte device key>
blob,data,hex2bin,<256-byte blob>
```

Template: `secrets.example.csv`

### Build and flash

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

---

## Architecture

```
app_main()
│
├── init_oled_display()       OLED driver, refresh task
├── init_uart()               UART console (serial menu)
├── init_settings_nvs_partition()
├── init_global_settings()    settings from NVS
├── read_secrets()            device keys from NVS
├── init_led_output()         GPIO 25 (advertising LED)
├── init_button_input()       PRG button, long-press shutdown
├── init_autobutton()         timed button-press queue
├── init_bluetooth()          Bluedroid, GATTS, GAP
└── pgp_advertise()           start BLE advertising
```

### Module overview

| Module                   | File(s)                          | Description |
|--------------------------|----------------------------------|-------------|
| Main program             | `pgpemu.c`                       | `app_main()`, boot sequence |
| BLE stack                | `pgp_bluetooth.c`                | Bluedroid init, MAC configuration |
| GAP (advertising)        | `pgp_gap.c`, `pgp_gap.h`         | start/stop BLE advertising, GAP events |
| GATTS (GATT server)      | `pgp_gatts.c`, `pgp_gatts.h`     | GATT services, connection events, notify |
| Handshake protocol       | `pgp_handshake.c`, `pgp_handshake_multi.c` | PGP authentication protocol, multi-client state |
| Cryptography             | `pgp_cert.c`, `pgp_cert.h`       | AES-CTR, challenge-response, certificate generation |
| LED handler              | `pgp_led_handler.c`              | interpret the app's LED patterns, trigger actions |
| Auto button              | `pgp_autobutton.c`               | timed button presses via queue |
| OLED display             | `oled_display.c`, `oled_display.h` | framebuffer, 5×7 font, display state |
| LED output               | `led_output.c`, `led_output.h`   | GPIO 25 advertising LED |
| Button                   | `button_input.c`, `button_input.h` | short/long press of the PRG button |
| Settings                 | `settings.c`, `settings.h`       | GlobalSettings and DeviceSettings with mutex |
| Statistics               | `stats.c`, `stats.h`             | caught/fled/spin per connection |
| Secrets                  | `secrets.c`, `config_secrets.c`  | MAC, device key, blob from NVS |
| NVS helpers              | `nvs_helper.c`                   | NVS read/write helpers |
| UART console             | `uart.c`                         | serial menu on UART0 |

---

## BLE protocol

The Pokémon GO Plus uses a proprietary GATT protocol over BLE:

1. **Advertising**: the device sends BLE advertisements with the cloned device name and MAC address.
2. **Connection**: the app connects and starts the handshake.
3. **Handshake (2 phases)**:
   - Phase 1: the device sends `challenge_data` (contains MAC, nonce, encrypted challenge, blob).
   - Phase 2: the app replies with the solved challenge; the device verifies it via AES and confirms.
4. **Session**: after a successful handshake the app enables notifications on the LED characteristic.
5. **Reconnect**: already-paired devices can re-authenticate faster via `reconnect_challenge`.

Up to **4 concurrent BLE connections** are supported (`CONFIG_BT_ACL_CONNECTIONS=4`).

---

## LED patterns and actions

The app sends LED animation commands from which the firmware derives the game state:

| Pattern                  | Meaning                 | Action              |
|--------------------------|-------------------------|---------------------|
| Green blinking only      | Pokémon in range        | press button (autocatch) |
| Yellow blinking only     | new Pokémon             | press button (autocatch) |
| Blue blinking only       | Pokéstop in range       | press button (autospin)  |
| Ball shake + blue/green  | Pokémon caught          | `caught++`, log     |
| Ball shake + red         | Pokémon fled            | `fled++`, log       |
| Red/green/blue cycling   | items received          | `spin++`, log       |
| White only               | bag full                | log                 |
| Red only (blinking)      | Pokéballs empty / OOR   | log                 |
| Red only (steady)        | box full                | log                 |

---

## OLED display

The display uses a 5×7 pixel bitmap font (6 px advance), upscaled for large text.
A short button press cycles through the pages:

| Page | Content |
|---|---|
| **1 Status** (default) | device name left, battery % right (only here); center, large: `CONNECTED` / `SEARCHING` / `IDLE`; bottom: the newest event across all clients with age (`Caught!  2m`) |
| **2 Stats** | `[k/n]` client rotation (every 3 s), `C/F/S` of the shown client, `Last C:`, below that the totals `All: C/F/S` |
| **3 Log** | the last 5 events across all clients, newest first, with client prefix (`1:Caught!`) and age |
| **4 Power** | voltage, percent, bar (1 s refresh) |
| **5 Debug** | uptime, VBAT raw/filt/min/max, sparkline (compile-time gated) |

```
┌─────────────────────────────┐
│ PKLMGOPLUS             85%  │  ← status page (page 1)
├─────────────────────────────┤
│                             │
│   C O N N E C T E D        │  ← state, large
│                             │
│ Caught!                 2m  │  ← newest event + age
└─────────────────────────────┘
```

Events (catch, flee, items, connect/disconnect, …) go into a global ring
(last 6, with client slot and timestamp) that feeds the status and log pages.
Age strings and battery percent refresh once per minute.

### Display sleep

After **20 min without a button press** the display counts down with a large
digit **5→1** and then turns off (~10 µA instead of ~5–8 mA, and it also
protects the OLED from burn-in). Any button press during the countdown aborts
it; when the display is off, **the first press only wakes the screen** (it
does not switch pages). Holding for 2–4 s still toggles advertising as usual,
even while the display is dark.

### OLED API

```c
void init_oled_display(void);
bool oled_activity(void);                          // button press: resets sleep timer, wakes panel
void oled_set_device_name(const char* name);       // device name (status page header)
void oled_set_connections(int count);              // number of active connections
void oled_set_advertising(bool adv);               // advertising state
void oled_set_event(uint16_t conn_id, const char* e); // event into the ring; unknown conn_id is dropped
void oled_show_banner(const char* c, const char* big, int ms); // overlay: caption + large text
void oled_clear_banner(void);
void oled_set_shutdown_countdown(int remaining);   // large seconds-remaining digit (0 = off)
void oled_shutdown(const char* msg);               // goodbye screen + display off
void oled_next_display(void);                      // next display page
```

The display works in a **pull model**: setters only mutate state and set a
dirty flag; rendering and the I²C flush happen exclusively in the refresh task
(250 ms tick). BLE callbacks are therefore never blocked by display I/O. The
caught/fled/spin counters are read directly from `stats.c` at render time
(`get_stats_snapshot`, `get_stats_totals`) — there is no parallel bookkeeping.

---

## Button controls

| Action               | Behavior                                         |
|----------------------|--------------------------------------------------|
| Short press (< 2 s)  | next display page                                |
| Hold 2–4 s           | toggle advertising: from 2 s the display shows the target state in large text ("Release to toggle: ADV OFF"), after release a 1.5 s confirmation ("ON"/"OFF") |
| Hold 4–8 s           | shutdown countdown: large digit 4→1 ("Shutdown in"), releasing aborts |
| Hold ≥ 8 s           | shutdown: show "Goodbye!", deep sleep (entered only after the button is released) |
| Wake from sleep      | short press of the PRG button (GPIO 0, ext0)     |

In deep sleep the current draw is about 10 µA. After waking, the firmware does a full restart (BLE pairing is required again).

---

## Settings

### GlobalSettings (NVS partition `global_settings`)

| Key       | Type | Meaning                              |
|-----------|------|--------------------------------------|
| `catch`   | i8   | autocatch enabled (1 = yes)          |
| `spin`    | i8   | autospin enabled (1 = yes)           |
| `llevel`  | i8   | log level (1=debug, 2=info, 3=verbose) |
| `maxcon`  | u8   | maximum concurrent connections       |

Values are loaded from NVS at boot. Changes made via the UART console are persisted to NVS.

### DeviceSettings (per connection)

Each connection has its own settings (`autocatch`, `autospin`), stored in `client_state_t`.

---

## Configuration (sdkconfig.defaults)

```
CONFIG_BT_ENABLED=y
CONFIG_BT_GATTS_ENABLE=y
CONFIG_BT_BLUEDROID_ENABLED=y
CONFIG_BT_CONTROLLER_MODE_BLE_ONLY=y

# Heltec WiFi Kit 32: 26 MHz crystal
CONFIG_XTAL_FREQ_26=y
CONFIG_XTAL_FREQ=26

# FreeRTOS trace (for vTaskList)
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS=y
```

---

## ESP32 port notes

The original code targeted the ESP32-C3. The following changes were required:

| Problem | Cause | Fix |
|---------|-------|-----|
| Display stays black | 26 MHz crystal instead of 40 MHz | `CONFIG_XTAL_FREQ_26=y` in sdkconfig.defaults |
| Watchdog reset (TG1WDT) | GPIO 8+9 are SPI flash pins on ESP32 | LED → GPIO 25, button → GPIO 0 |
| Linker error USB-JTAG | `usb_serial_jtag_*` does not exist on ESP32 | UART0 driver in `uart.c` |
| `aes/esp_aes.h` not found | `mbedtls` missing from CMakeLists REQUIRES | added |
| `vTaskList` undefined | FreeRTOS trace not enabled | in sdkconfig.defaults + `#if` guard |
| `[3/2]` impossible | `snprintf` truncation warning | direct char-array construction for the slot display |
