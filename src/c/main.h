#include <pebble.h>

#define RADIUS_1 81              // radius for the outermost circle
#define RADIUS_2 72              // radius for the inner circle
#define RECORD_1_3 800           // reference anchor data point for 1/3 completion
#define RECORD_2_3 1100          // reference anchor data point for 2/3 completion
#define RECORD_3_3 1300          // reference anchor data point for 3/3 completion
#define NUM_COLOR_POITNS 1440    // total number of data points (assume there are 720 points on each circle)

extern Window *window;
extern bool bShowCount;

GPoint get_point_on_polar_from_r(GRect bounds, int radius, float angle);