#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "clock_common.h"

// Initialise UART2 and LOAD GPIO.
void dotmatrix_init(void);

// Clear the internal frame buffer (all dots off).
void dotmatrix_clear(void);

// Set / clear a single pixel in the frame buffer.
void dotmatrix_set_pixel(int col, int row, bool on);

// Set all 8 dots in one column (bit7 = row0 top, bit0 = row7 bottom).
void dotmatrix_set_col(int col, uint8_t dots);

// Render a 5-column font glyph into digit_slot (0-3).
// glyph_cols[5]: each element is a 7-bit column bitmap (bit6=top, bit0=bottom).
void dotmatrix_draw_glyph(int digit_slot, const uint8_t glyph_cols[FONT_COLS]);

// Set/clear the colon between digit_slot and digit_slot+1.
// The colon sits on the annotation column (col = slot*8 + COLON_COL_OFFSET).
void dotmatrix_set_colon(int left_digit_slot, bool on);

// Set/clear the alarm indicator dot (row 0, col 31 – upper-right corner).
void dotmatrix_set_alarm_dot(bool on);

// Serialise the frame buffer over UART2 as ASCII, pulse LOAD to latch,
// then log a summary line showing what is on screen.
//   label       – human-readable display content, e.g. "01:23" or "--:--" or "1   "
//   alarm_on    – current alarm armed state (drives the summary log)
void dotmatrix_flush(const char *label, bool alarm_on);
