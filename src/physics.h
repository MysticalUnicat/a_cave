#pragma once

#include "util.h"
#include "transform.h"

#include <chipmunk/chipmunk.h>

DECLARE_COMPONENT(Body2D, {
  enum {
    Body2D_dynamic,
    Body2D_kinematic,
    Body2D_static
  } kind;
  float mass;
  float moment;
  cpBody * body;
});

struct Collision2D_data {
  enum {
    Collision2D_circle,
    Collision2D_box
  } kind;
  uint32_t collision_type;
  float radius;
  float width;
  float height;
  bool sensor;
  float elasticity;
  float friction;
};

DECLARE_COMPONENT(Collision2D, {
  Entity body;
  struct Collision2D_data * data;
  cpShape * shape;
});

void physics_frame(void);
