#pragma once
#include "pebble.h"
#define STEP_GOAL 500


static const GPathInfo MINUTE_HAND_POINTS = {
  6, (GPoint []) {
    {-2, 2},
    {2, 2},
    {2, -47},
    {1,-50},
    {-1,-50},
    {-2, -47}
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
  6, (GPoint []){
    {-3, 3},
    {3, 3},
    {3, -37},
    {1, -39},
    {-1, -39},
    {-3, -37}
  }
};
