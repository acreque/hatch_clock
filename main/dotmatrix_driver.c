#include "dotmatrix_driver.h"
#include "clock_common.h"

#include <stdio.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "dotmatrix";

// Internal frame buffer: framebuf[col] = 8-bit dot pattern
// bit7 = row 0 (top), bit0 = row 7 (bottom)
static uint8_t framebuf[DISPLAY_COLS];

// ─── Initialisation ───────────────────────────────────────────────────────────

void dotmatrix_init(void)
{
    const uart_config_t uart_cfg = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_TX_PIN, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, 256, 256, 0, NULL, 0));

    // Configure LOAD pin (GPIO16) as output, initially low.
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DOTMATRIX_LOAD_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level(DOTMATRIX_LOAD_PIN, 0);

    dotmatrix_clear();
    dotmatrix_flush("(init)", false);

    ESP_LOGI(TAG, "Dot-matrix driver initialised (UART2 TX=GPIO%d, LOAD=GPIO%d)",
             UART_TX_PIN, DOTMATRIX_LOAD_PIN);
}

// ─── Frame-buffer helpers ─────────────────────────────────────────────────────

void dotmatrix_clear(void)
{
    for (int c = 0; c < DISPLAY_COLS; c++) {
        framebuf[c] = 0x00;
    }
}

void dotmatrix_set_pixel(int col, int row, bool on)
{
    if (col < 0 || col >= DISPLAY_COLS || row < 0 || row >= DISPLAY_ROWS) return;
    if (on) {
        framebuf[col] |=  (uint8_t)(0x80u >> row);
    } else {
        framebuf[col] &= ~(uint8_t)(0x80u >> row);
    }
}

void dotmatrix_set_col(int col, uint8_t dots)
{
    if (col < 0 || col >= DISPLAY_COLS) return;
    framebuf[col] = dots;
}

// ─── Glyph rendering ─────────────────────────────────────────────────────────

void dotmatrix_draw_glyph(int digit_slot, const uint8_t glyph_cols[FONT_COLS])
{
    int base_col = digit_slot * DIGIT_WIDTH;

    for (int fc = 0; fc < FONT_COLS; fc++) {
        int col = base_col + fc;
        uint8_t dots = 0;
        for (int row = 0; row < FONT_ROWS; row++) {
            if (glyph_cols[fc] & (0x40u >> row)) {
                dots |= (uint8_t)(0x80u >> (row + FONT_ROW_OFFSET));
            }
        }
        framebuf[col] = dots;
    }
}

void dotmatrix_set_colon(int left_digit_slot, bool on)
{
    int col = left_digit_slot * DIGIT_WIDTH + COLON_COL_OFFSET;
    uint8_t dots = on ? ((0x80u >> 2) | (0x80u >> 5)) : 0x00;
    framebuf[col] = dots;
}

void dotmatrix_set_alarm_dot(bool on)
{
    dotmatrix_set_pixel(DISPLAY_COLS - 1, 0, on);
}

// ─── Flush frame buffer to hardware ──────────────────────────────────────────

void dotmatrix_flush(const char *label, bool alarm_on)
{
    // Send all 32 column messages over UART2 as ASCII.
    // Format per column: "C<cc>:0x<HH>\r\n"
    //   cc = zero-padded 2-digit decimal column number (00-31)
    //   HH = 2-digit uppercase hex dot pattern
    // Example: "C06:0x24\r\n"
    char msg[14];
    for (int c = 0; c < DISPLAY_COLS; c++) {
        int len = snprintf(msg, sizeof(msg), "C%02d:0x%02X\r\n", c, framebuf[c]);
        uart_write_bytes(UART_NUM, msg, len);
    }

    // Wait for all bytes to leave the TX FIFO before latching.
    uart_wait_tx_done(UART_NUM, pdMS_TO_TICKS(100));

    // Pulse LOAD high then low to latch the buffer into the display.
    gpio_set_level(DOTMATRIX_LOAD_PIN, 1);
    gpio_set_level(DOTMATRIX_LOAD_PIN, 0);

    // Log a one-line summary of what was just sent to the display.
    ESP_LOGI(TAG, "display=[%s] alarm=%s", label, alarm_on ? "ON" : "OFF");
}
