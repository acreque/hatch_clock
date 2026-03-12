#pragma once

// FreeRTOS task function for clock state management.
// Handles: timekeeping, button debounce, display-mode transitions.
void clock_task(void *arg);
