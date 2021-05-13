#include "physics.h"

DEFINE_COMPONENT(Body2D)

DEFINE_COMPONENT(Collision2D)

DEFINE_COMPONENT(Constraint2D)

LAZY_GLOBAL(cpSpace *, physics_space, inner = cpSpaceNew();)

static float _speed = 1.0f;

void physics_set_speed(float speed) {
  _speed = speed;
}

static void _cleanup_constraint(cpSpace * space, void * key, void * data) {
  cpConstraint * c = (cpConstraint *)data;
  cpSpaceRemoveConstraint(space, c);
  cpConstraintFree(c);
}

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
      if(c->inactive) {
        cpSpaceAddPostStepCallback(physics_space(), _cleanup_constraint, c->constraint, c->constraint);
        c->constraint = NULL;
      }
      return;
    }

    if(c->inactive) {
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
  const float timestep = 1.0f / 60.0f;
  
  static float p_time = 0.0f;
  static float s_time = 0.0f;

  s_time += GetFrameTime() * _speed;

  while(p_time < s_time) {
    cpSpaceStep(physics_space(), timestep);
    p_time += timestep;
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

struct ApplyImpulseData {
  cpVect i;
  cpVect p;
};
void _apply_impulse(cpSpace * space, void * key, void * _data) {
  struct ApplyImpulseData * data = (struct ApplyImpulseData *)_data;
  cpBodyApplyImpulseAtLocalPoint((cpBody *)key, data->i, data->p);
}
void _apply_impulse_free(cpSpace * space, void * key, void * _data) {
  _apply_impulse(space, key, _data);
  free(_data);
}

void physics_apply_impulse(Entity e, cpVect i, cpVect p) {
  struct Body2D * b = Body2D_write(e);
  if(b->body != NULL) {
    if(cpSpaceIsLocked(physics_space())) {
      struct ApplyImpulseData * data = malloc(sizeof(*data));
      data->i = i;
      data->p = p;
      cpSpaceAddPostStepCallback(physics_space(), _apply_impulse_free, b->body, data);
    } else {
      struct ApplyImpulseData data;
      data.i = i;
      data.p = p;
      _apply_impulse(physics_space(), b->body, &data);
    }
  }
}

void physics_deactivate_constraint(Entity e) {
  Constraint2D_write(e)->inactive = true;
}

