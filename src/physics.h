#pragma once

#define PHYSICS_USE_ALIAS 1

#if PHYSICS_USE_ALIAS
#include <alias/physics.h>
#include "transform.h"

extern alias_Physics2DBundle g_physics_bundle;

static inline alias_ecs_ComponentHandle alias_Physics2DLinearMotion_component(void) {
  return g_physics_bundle.Physics2DLinearMotion_component;
}

static inline alias_Physics2DLinearMotion * alias_Physics2DLinearMotion_write(alias_ecs_EntityHandle e) {
  alias_Physics2DLinearMotion * result;
  alias_ecs_write_entity_component(g_world, e, g_physics_bundle.Physics2DLinearMotion_component, (void **)&result);
  return result;
}

static inline alias_ecs_ComponentHandle alias_Physics2DLinearMass_component(void) {
  return g_physics_bundle.Physics2DLinearMass_component;
}

void physics_init(void);
void physics_transform_update(alias_R speed);
#else
#include "util.h"
#include "transform.h"
#include "math.h"

#include <chipmunk/chipmunk.h>

struct Physics_material {
  uint32_t category;
  float elasticity;
  float friction;
  alias_Vector2D surface_velocity;
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

DECLARE_COMPONENT(Velocity2D, {
  union {
    struct {
      float x;
      float y;
      float a;
    };
    struct {
      alias_Vector2D velocity;
      float angular_velocity;
    };
  };
});

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
  alias_Point2D anchor_a;
  alias_Point2D anchor_b;
  float min;
  float max;

  bool inactive;
  cpConstraint * constraint;
});

DECLARE_COMPONENT(AddImpulse2D, {
  Entity body;
  alias_Vector2D impulse;
  alias_Point2D point;
});

DECLARE_COMPONENT(Contact2D, {
  enum {
    Contact2D_begin,
    Contact2D_seperate
  } kind;
  uint32_t type_a;
  uint32_t type_b;
  Entity body_a;
  Entity body_b;
  Entity shape_a;
  Entity shape_b;
  alias_Vector2D velocity;
  alias_Vector2D normal;
});

cpSpace * physics_space(void);
void physics_set_speed(float speed);
void physics_set_gravity(float x, float y);
void physics_set_damping(float d);
void physics_update(void);
#endif

