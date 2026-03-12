#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// ─── Hardware Pin Definitions ─────────────────────────────────────────────────
#define UART_NUM            UART_NUM_2
#define UART_TX_PIN         17          // UART2 TX (GPIO17)
#define UART_RX_PIN         16          // UART2 RX – repurposed; LOAD uses GPIO16
#define DOTMATRIX_LOAD_PIN  16          // GPIO16 → LOAD latch signal
#define BTN1_PIN            34          // Button 1: show alarm time for 5s
#define BTN2_PIN            35          // Button 2: toggle alarm on/off, show indicator

// ─── Display Geometry ─────────────────────────────────────────────────────────
//  Total display:  32 columns × 8 rows
//  Each digit:     8 columns × 8 rows
//    - usable font area:  5 wide × 7 tall  (cols 0-4, rows 1-7)
//    - annotation cols:   cols 5-7 (right margin)
//    - annotation row:    row 0 (top margin)
//  Colon separator:  middle of the 3 annotation cols of each digit boundary
//  Digit 0 → cols  0- 7
//  Digit 1 → cols  8-15
//  Digit 2 → cols 16-23
//  Digit 3 → cols 24-31
//  Alarm dot:  row 0, col 31 (upper-right corner of display)

#define DISPLAY_COLS        32
#define DISPLAY_ROWS        8
#define DIGIT_WIDTH         8
#define FONT_COLS           5
#define FONT_ROWS           7
#define FONT_ROW_OFFSET     1           // font starts at row 1 (row 0 is annotation)
#define COLON_COL_OFFSET    6           // col 6 within digit block = middle of 3 annotation cols

// ─── Startup Defaults ─────────────────────────────────────────────────────────
#define INIT_CLOCK_HOUR     1
#define INIT_CLOCK_MIN      23
#define INIT_ALARM_HOUR     4
#define INIT_ALARM_MIN      56
#define INIT_ALARM_STATE    false       // alarm off at power-up

// ─── Timing ───────────────────────────────────────────────────────────────────
#define BUTTON_SHOW_MS      5000        // How long button-triggered display lasts
#define DEBOUNCE_MS         50          // Button debounce window

// ─── Display Mode (shared between tasks) ──────────────────────────────────────
typedef enum {
    DISP_MODE_CLOCK,        // Normal HH:MM clock display
    DISP_MODE_ALARM,        // Show alarm time (or --:-- if off) for 5s
    DISP_MODE_BTN2,         // Show "1   " pattern + alarm dot for 5s
} display_mode_t;

// ─── Shared State (protected by mutex) ────────────────────────────────────────
typedef struct {
    uint8_t         clock_hour;
    uint8_t         clock_min;
    uint8_t         clock_sec;
    uint8_t         alarm_hour;
    uint8_t         alarm_min;
    bool            alarm_enabled;
    display_mode_t  display_mode;
    TickType_t      mode_expire_tick;   // When to revert to DISP_MODE_CLOCK
} clock_state_t;

extern clock_state_t g_clock_state;
extern SemaphoreHandle_t g_state_mutex;

// ─── UART serial protocol to dot-matrix driver ────────────────────────────────
// Each message: [col_byte, dots_byte]
//   col_byte  = column number 0-31
//   dots_byte = 8-bit dot pattern, bit7 = row0 (top), bit0 = row7 (bottom)
// After all 32 columns are sent, pulse LOAD high then low to latch.
