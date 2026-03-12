# Hatch Dot-Matrix Alarm Clock

ESP32 firmware for a 32×8 dot-matrix LED alarm clock display.

---

## Hardware Connections

| Signal       | ESP32 GPIO | Notes                                    |
|--------------|-----------|------------------------------------------|
| UART2 TX     | GPIO 17   | Serial data to dot-matrix driver          |
| LOAD (latch) | GPIO 16   | Pulse HIGH→LOW to latch all 32 columns    |
| BTN1         | GPIO 34   | Active-low, internal pull-up; show alarm  |
| BTN2         | GPIO 35   | Active-low, internal pull-up; toggle alarm|

> **Note:** GPIO 34 and 35 are input-only pins on ESP32 – suitable for buttons.

---

## Display Layout

```
 Digit 0   Digit 1   Digit 2   Digit 3
[col 0-7] [col 8-15][col 16-23][col 24-31]

Each digit block:
  cols 0-4  = 5-wide font glyph
  col  5    = annotation (unused)
  col  6    = colon dots (between digit pairs 1 and 2)
  col  7    = annotation (unused)

Row 0 of col 31 = alarm indicator dot (upper-right corner)
```

### Serial Protocol

Each display update sends **32 two-byte messages** over UART2:

```
Byte 0: column number  (0x00 – 0x1F)
Byte 1: dot pattern    (bit7 = row 0 top, bit0 = row 7 bottom)
```

After all 32 columns are sent, **LOAD** is pulsed HIGH then LOW to latch.

---

## Behaviour

| Event           | Display                             | Alarm dot         |
|-----------------|-------------------------------------|-------------------|
| Power-up        | `01:23` (clock starts at 1:23)      | OFF               |
| Normal run      | `HH:MM` updating every second, colon blinks | Reflects alarm state |
| BTN1 press      | Alarm time `04:56` for 5 s; if alarm off: `--:--` | Reflects alarm state |
| BTN2 press      | `1   ` (digit 0 = "1", rest blank) for 5 s | Reflects **toggled** state |
| After 5 s       | Returns to normal clock             | —                 |

---

## Build & Flash (VS Code + ESP-IDF Extension)

1. Open this folder in VS Code.
2. The ESP-IDF extension will detect `CMakeLists.txt` automatically.
3. Select your target: `ESP32`.
4. Click **Build** (or `Ctrl+Shift+B`).
5. Click **Flash** and select the correct COM port.

Alternatively, using the command line:

```bash
idf.py set-target esp32
idf.py build
idf.py -p /dev/cu.usbserial-1430 flash monitor
```

---

## Project Structure

```
hatch-dotmatrix/
├── CMakeLists.txt          # Root build file
├── sdkconfig.defaults      # Default Kconfig options
├── README.md
└── main/
    ├── CMakeLists.txt      # Component sources
    ├── main.c              # app_main: init + task spawn
    ├── clock_common.h      # Shared types, pin defs, globals
    ├── clock_task.c/h      # Task 1: timekeeping + buttons + mode FSM
    ├── display_task.c/h    # Task 2: rendering engine
    ├── dotmatrix_driver.c/h# UART2 + LOAD GPIO driver, frame-buffer API
    ├── font5x7.c/h         # 5×7 bitmap font (digits 0-9, dash, blank)
    └── ...
```
