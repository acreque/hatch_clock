#include "clock_task.h"
#include "clock_common.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "clock_task";

// ─── Shared state (defined here, extern in clock_common.h) ───────────────────
clock_state_t     g_clock_state;
SemaphoreHandle_t g_state_mutex;

// ─── Button helpers ───────────────────────────────────────────────────────────

static bool button_read_debounced(gpio_num_t pin)
{
    // Buttons are active-low with internal pull-up.
    if (gpio_get_level(pin) != 0) return false;       // not pressed
    vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
    return (gpio_get_level(pin) == 0);                 // still pressed?
}

static void buttons_init(void)
{
    // GPIO 34 and 35 are input-only pads on ESP32 and have no internal
    // pull-up or pull-down resistors.  Attempting to enable them causes a
    // runtime gpio error.  External pull-ups (e.g. 10 kΩ to 3.3 V) must be
    // fitted on the PCB; buttons are wired active-low to GND as before.
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BTN1_PIN) | (1ULL << BTN2_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
}

// ─── Clock task ───────────────────────────────────────────────────────────────

void clock_task(void *arg)
{
    ESP_LOGI(TAG, "Clock task started");

    buttons_init();

    // Tick reference for 1-second clock updates.
    TickType_t last_tick = xTaskGetTickCount();

    while (1) {
        // ── 1. Poll buttons (non-blocking, 10 ms polling interval) ───────────

        if (button_read_debounced(BTN1_PIN)) {
            ESP_LOGI(TAG, "BTN1 pressed – showing alarm time");
            xSemaphoreTake(g_state_mutex, portMAX_DELAY);
            g_clock_state.display_mode    = DISP_MODE_ALARM;
            g_clock_state.mode_expire_tick = xTaskGetTickCount() +
                                             pdMS_TO_TICKS(BUTTON_SHOW_MS);
            xSemaphoreGive(g_state_mutex);
            // Wait for button release to avoid re-triggering.
            while (gpio_get_level(BTN1_PIN) == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }

        if (button_read_debounced(BTN2_PIN)) {
            ESP_LOGI(TAG, "BTN2 pressed – toggling alarm");
            xSemaphoreTake(g_state_mutex, portMAX_DELAY);
            g_clock_state.alarm_enabled   = !g_clock_state.alarm_enabled;
            g_clock_state.display_mode    = DISP_MODE_BTN2;
            g_clock_state.mode_expire_tick = xTaskGetTickCount() +
                                             pdMS_TO_TICKS(BUTTON_SHOW_MS);
            xSemaphoreGive(g_state_mutex);
            while (gpio_get_level(BTN2_PIN) == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }

        // ── 2. Check for mode expiry ─────────────────────────────────────────

        xSemaphoreTake(g_state_mutex, portMAX_DELAY);
        if (g_clock_state.display_mode != DISP_MODE_CLOCK) {
            if (xTaskGetTickCount() >= g_clock_state.mode_expire_tick) {
                g_clock_state.display_mode = DISP_MODE_CLOCK;
                ESP_LOGI(TAG, "Returning to clock display");
            }
        }
        xSemaphoreGive(g_state_mutex);

        // ── 3. Advance clock every 1 second ──────────────────────────────────

        TickType_t now = xTaskGetTickCount();
        if ((now - last_tick) >= pdMS_TO_TICKS(1000)) {
            last_tick += pdMS_TO_TICKS(1000);

            xSemaphoreTake(g_state_mutex, portMAX_DELAY);

            g_clock_state.clock_sec++;
            if (g_clock_state.clock_sec >= 60) {
                g_clock_state.clock_sec = 0;
                g_clock_state.clock_min++;
                if (g_clock_state.clock_min >= 60) {
                    g_clock_state.clock_min = 0;
                    g_clock_state.clock_hour++;
                    if (g_clock_state.clock_hour >= 24) {
                        g_clock_state.clock_hour = 0;
                    }
                }
            }

            xSemaphoreGive(g_state_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // 10 ms polling granularity
    }
}
