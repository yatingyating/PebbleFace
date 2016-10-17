#include <pebble.h>
#include "main.h"

extern Window * step_window;
extern int step_sum;

void step_window_init();
void step_window_deinit();
void refresh_step_window(float hour);