#pragma once

#include "util.h"
#include "transform.h"
#include "math.h"

#include <chipmunk/chipmunk.h>

struct Physics_material {
  uint32_t category;
  float elasticity;
  float friction;
  vector2 surface_velocity;
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

DECLARE_COMPONENT(Collision2D, {
  Entity body;
  enum {
    Collision2D_circle,
    Collision2D_box
  } kind;
  float radius;
  float width;
  float height;
  uint32_t collision_type;
  bool sensor;
  float elasticity;
  float friction;

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
  point2 anchor_a;
  point2 anchor_b;
  float min;
  float max;

  bool inactive;
  cpConstraint * constraint;
});

DECLARE_COMPONENT(AddImpulse2D, {
  Entity body;
  vector2 impulse;
  point2 point;
});

DECLARE_COMPONENT(Contact2D, {
  uint32_t type_a;
  uint32_t type_b;
  Entity body_a;
  Entity body_b;
  Entity shape_a;
  Entity shape_b;
  vector2 velocity;
  normal2 normal;
});

cpSpace * physics_space(void);
void physics_set_speed(float speed);
void physics_set_gravity(float x, float y);
void physics_set_damping(float d);
void physics_update(void);

