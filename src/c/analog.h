#pragma once
#include "pebble.h"
#define STEP_GOAL 500


static const GPathInfo MINUTE_HAND_POINTS = {
  6, (GPoint []) {
    {-2, 2},
    {2, 2},
    {2, -55},
    {1,-57},
    {-1,-57},
    {-2, -55}
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
  6, (GPoint []){
    {-3, 3},
    {3, 3},
    {3, -42},
    {1, -44},
    {-1, -44},
    {-3, -42}
  }
};