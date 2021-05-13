#include "physics.h"

DEFINE_COMPONENT(Body2D)

DEFINE_COMPONENT(Collision2D)

DEFINE_COMPONENT(Constraint2D)

LAZY_GLOBAL(cpSpace *, physics_space, inner = cpSpaceNew();)

static void _create_new_bodies(void) {
  QUERY(
      ( read, Transform2D, t )
    , ( write, Body2D, b )
  ) {
    if(b->body != NULL) {
      return;
    }

    switch(b->kind) {
    case Body2D_dynamic:
      b->body = cpBodyNew(b->mass, b->moment);
      break;
    case Body2D_kinematic:
      b->body = cpBodyNewKinematic();
      break;
    case Body2D_static:
      b->body = cpBodyNewStatic();
      break;
    }

    cpBodySetPosition(b->body, (cpVect) { t->x, t->y });

    cpSpaceAddBody(physics_space(), b->body);
  }

  QUERY(
      ( write, Body2D, b )
    , ( write, Collision2D, c )
  ) {
    if(c->shape != NULL || b->body == NULL) {
      return;
    }

    switch(c->data->kind) {
    case Collision2D_circle:
      c->shape = cpCircleShapeNew(b->body, c->data->radius, cpv(0, 0));
      break;
    case Collision2D_box:
      c->shape = cpBoxShapeNew(b->body, c->data->width, c->data->height, c->data->radius);
      break;
    }

    if(c->shape == NULL) {
      return;
    }

    cpShapeSetSensor(c->shape, c->data->sensor);
    cpShapeSetElasticity(c->shape, c->data->elasticity);
    cpShapeSetFriction(c->shape, c->data->friction);
    cpShapeSetCollisionType(c->shape, c->data->collision_type);

    cpSpaceAddShape(physics_space(), c->shape);
  }

  QUERY(
      ( write, Constraint2D, c )
  ) {
    if(c->constraint != NULL) {
      return;
    }

    cpBody * a = Body2D_write(c->body_a)->body;
    cpBody * b = Body2D_write(c->body_b)->body;

    switch(c->kind) {
    case Constraint2D_pin:
      c->constraint = cpPinJointNew(a, b, cpv(c->anchor_a[0], c->anchor_a[1]), cpv(c->anchor_b[0], c->anchor_b[1]));
      break;
    case Constraint2D_rotary:
      c->constraint = cpRotaryLimitJointNew(a, b, c->min, c->max);
      break;
    }

    cpSpaceAddConstraint(physics_space(), c->constraint);
  }
}

static void _prepare_kinematic(void) {
  QUERY(
      ( write, Velocity2D, v )
    , ( write, Body2D, b )
  ) {
    if(b->body == NULL || b->kind != Body2D_kinematic) {
      return;
    }

    cpBodySetVelocity(b->body, cpv(v->x, v->y));
    cpBodySetAngularVelocity(b->body, v->a);

    v->x = 0.0f;
    v->y = 0.0f;
    v->a = 0.0f;
  }
}

static void _iterate(void) {
  static float accum = 0.0f;
  const float timestep = 1.0f / 60.0f;
  
  float time = GetTime();

  while(accum < time) {
    cpSpaceStep(physics_space(), timestep);
    accum += timestep;
  }
}

static void _update_transform(void) {
  QUERY(
      ( write, Transform2D, t )
    , ( read, Body2D, b )
  ) {
    if(b->body == NULL) {
      return;
    }

    cpVect p = cpBodyGetPosition(b->body);
    t->x = p.x;
    t->y = p.y;
    t->a = cpBodyGetAngle(b->body);
  }
}

void physics_frame(void) {
  _create_new_bodies();
  _prepare_kinematic();
  _iterate();
  _update_transform();
}

