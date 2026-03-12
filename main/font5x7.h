#pragma once
#include <stdint.h>
#include "clock_common.h"

// Glyph indices
#define FONT5X7_DIGIT(n)    (n)         // 0-9  → digits
#define FONT5X7_DASH        10          // '-'
#define FONT5X7_BLANK       11          // ' '
#define FONT5X7_GLYPH_COUNT 12

// font5x7[glyph][col] = 7-bit column bitmap, bit6 = top row, bit0 = bottom row
extern const uint8_t font5x7[FONT5X7_GLYPH_COUNT][FONT_COLS];
