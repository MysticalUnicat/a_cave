#pragma once

#include "util.h"
#include "math.h"

DECLARE_COMPONENT(Transform2D, {
  union {
    struct {
      float x;
      float y;
      float a;
    };
    struct {
      v2 position;
      float angle;
    };
  };
});

DECLARE_COMPONENT(Velocity2D, {
  union {
    struct {
      float x;
      float y;
      float a;
    };
    struct {
      n2 velocity;
      float angular_velocity;
    };
  };
});

