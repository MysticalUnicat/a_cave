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
      point2 position;
      float angle;
    };
  };
});

static const struct Transform2D Transform2D_zero = { 0.0f, 0.0f, 0.0f };

DECLARE_COMPONENT(Velocity2D, {
  union {
    struct {
      float x;
      float y;
      float a;
    };
    struct {
      vector2 velocity;
      float angular_velocity;
    };
  };
});

