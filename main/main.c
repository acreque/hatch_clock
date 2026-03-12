#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "clock_common.h"
#include "clock_task.h"
#include "display_task.h"
#include "dotmatrix_driver.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Hatch dot-matrix alarm clock starting");

    // ── Initialise shared state ───────────────────────────────────────────────
    memset(&g_clock_state, 0, sizeof(g_clock_state));

    g_clock_state.clock_hour     = INIT_CLOCK_HOUR;
    g_clock_state.clock_min      = INIT_CLOCK_MIN;
    g_clock_state.clock_sec      = 0;
    g_clock_state.alarm_hour     = INIT_ALARM_HOUR;
    g_clock_state.alarm_min      = INIT_ALARM_MIN;
    g_clock_state.alarm_enabled  = INIT_ALARM_STATE;
    g_clock_state.display_mode   = DISP_MODE_CLOCK;
    g_clock_state.mode_expire_tick = 0;

    // ── Create mutex ──────────────────────────────────────────────────────────
    g_state_mutex = xSemaphoreCreateMutex();
    configASSERT(g_state_mutex != NULL);

    // ── Initialise dot-matrix hardware driver ─────────────────────────────────
    dotmatrix_init();

    // ── Spawn tasks ───────────────────────────────────────────────────────────
    //
    // clock_task  – manages timekeeping, buttons, display-mode transitions
    //               Priority 5 (higher) so time ticks are not starved.
    //
    // display_task – renders the display from shared state at 500 ms intervals
    //               Priority 4 (lower) since it can afford to wake late by a
    //               few ms without affecting correctness.

    BaseType_t ret;

    ret = xTaskCreate(clock_task,
                      "clock_task",
                      4096,
                      NULL,
                      5,
                      NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(display_task,
                      "display_task",
                      4096,
                      NULL,
                      4,
                      NULL);
    configASSERT(ret == pdPASS);

    ESP_LOGI(TAG, "All tasks started. Clock: %02d:%02d  Alarm: %02d:%02d (%s)",
             INIT_CLOCK_HOUR, INIT_CLOCK_MIN,
             INIT_ALARM_HOUR, INIT_ALARM_MIN,
             INIT_ALARM_STATE ? "ON" : "OFF");

    // app_main returns – the FreeRTOS scheduler continues running the tasks.
}
