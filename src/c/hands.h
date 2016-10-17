#include <pebble.h>
#include "stepcount.h"

extern Layer *graphics_layer;

extern int hour;
extern int minutes;
extern int seconds;
extern float hour_angle;

void hands_init();
void hands_deinit();
GPoint get_point_on_polar(GRect bounds, float radiusFraction, float angle);
