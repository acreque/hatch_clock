#pragma once

// FreeRTOS task function responsible solely for rendering content on the
// dot-matrix display.  Reads shared state (g_clock_state) and calls the
// dotmatrix driver to push each frame.
void display_task(void *arg);
