#include "display_task.h"
#include "clock_common.h"
#include "dotmatrix_driver.h"
#include "font5x7.h"

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "display_task";

// ─── Internal helpers ─────────────────────────────────────────────────────────

static void render_digit(int slot, int digit)
{
    int glyph_idx = (digit >= 0 && digit <= 9) ? FONT5X7_DIGIT(digit) : FONT5X7_DASH;
    dotmatrix_draw_glyph(slot, font5x7[glyph_idx]);
}

static void render_blank(int slot)
{
    dotmatrix_draw_glyph(slot, font5x7[FONT5X7_BLANK]);
}

// Render HH:MM (or --:-- if hour/min < 0).
static void render_time(int hour, int min, bool alarm_dot, bool colon_on)
{
    dotmatrix_clear();

    char label[24];   // generously sized to satisfy -Werror=format-truncation
    if (hour < 0 || min < 0) {
        for (int slot = 0; slot < 4; slot++) {
            dotmatrix_draw_glyph(slot, font5x7[FONT5X7_DASH]);
        }
        snprintf(label, sizeof(label), "--:--");
    } else {
        render_digit(0, hour / 10);
        render_digit(1, hour % 10);
        render_digit(2, min / 10);
        render_digit(3, min % 10);
        snprintf(label, sizeof(label), "%02d:%02d", hour, min);
    }

    dotmatrix_set_colon(1, colon_on);
    dotmatrix_set_alarm_dot(alarm_dot);
    dotmatrix_flush(label, alarm_dot);
}

// Render BTN2 acknowledgement: digit 0 = "1", rest blank.
static void render_btn2(bool alarm_enabled)
{
    dotmatrix_clear();
    dotmatrix_draw_glyph(0, font5x7[FONT5X7_DIGIT(1)]);
    render_blank(1);
    render_blank(2);
    render_blank(3);
    dotmatrix_set_colon(1, false);
    dotmatrix_set_alarm_dot(alarm_enabled);
    dotmatrix_flush("1   ", alarm_enabled);
}

// ─── Display task ─────────────────────────────────────────────────────────────

void display_task(void *arg)
{
    ESP_LOGI(TAG, "Display task started");

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        xSemaphoreTake(g_state_mutex, portMAX_DELAY);
        clock_state_t state = g_clock_state;
        xSemaphoreGive(g_state_mutex);

        switch (state.display_mode) {

            case DISP_MODE_CLOCK:
                render_time(state.clock_hour, state.clock_min,
                            state.alarm_enabled,
                            (state.clock_sec % 2) == 0);
                break;

            case DISP_MODE_ALARM:
                if (state.alarm_enabled) {
                    render_time(state.alarm_hour, state.alarm_min,
                                state.alarm_enabled, true);
                } else {
                    render_time(-1, -1, false, false);
                }
                break;

            case DISP_MODE_BTN2:
                render_btn2(state.alarm_enabled);
                break;
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(500));
    }
}
