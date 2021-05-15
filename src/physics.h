#pragma once

#include "util.h"
#include "transform.h"

#include <chipmunk/chipmunk.h>

struct Physics_material {
  uint32_t category;
  float elasticity;
  float friction;
  struct {
    float x;
    float y;
  } surface_velocity;
};

struct Physics_collision_data {
  Entity body_a;
  Entity shape_a;
  
  Entity body_b;
  Entity shape_b;


};

struct Physics_collision {
  enum {
    Physics_collision_begin,
    Physics_collision_pre_solve,
    Physics_collision_post_solve,
    Physics_collision_seperate
  } kind;
  uint32_t category_a;
  uint32_t category_b;
  void * user_data;
  union {
    bool (* begin)(struct Physics_begin_data * data, void * user_data);
  };
};

DECLARE_COMPONENT(Body2D, {
  enum {
    Body2D_dynamic,
    Body2D_kinematic,
    Body2D_static
  } kind;
  float mass;
  float moment;

  bool inactive;
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

  bool inactive;
  cpShape * shape;
});

DECLARE_COMPONENT(Constraint2D, {
  enum {
    Constraint2D_pin,
    Constraint2D_rotary
  } kind;
  Entity body_a;
  Entity body_b;
  float anchor_a[2];
  float anchor_b[2];
  float min;
  float max;

  bool inactive;
  cpConstraint * constraint;
});

DECLARE_COMPONENT(AddImpulse2D, {
  Entity body;
  float impulse[2];
  float point[2];
});

cpSpace * physics_space(void);
void physics_set_speed(float speed);
void physics_update(void);

