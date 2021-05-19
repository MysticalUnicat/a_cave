#include "physics.h"
#include "event.h"

DEFINE_COMPONENT(Body2D)

DEFINE_COMPONENT(Collision2D)

DEFINE_COMPONENT(Constraint2D)

DEFINE_COMPONENT(AddImpulse2D)

LAZY_GLOBAL(cpSpace *, physics_space, inner = cpSpaceNew();)

static float _speed = 1.0f;

void physics_set_speed(float speed) {
  _speed = speed;
}

static void _create_bodies(void) {
  QUERY(
      ( write, Body2D, b )
    , ( filter, exclude, Transform2D )
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

    cpSpaceAddBody(physics_space(), b->body);
  }

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
}

static void _new_shape(cpBody * body, struct Collision2D * c, float x, float y, float a) {
  cpVect box[4];
  cpFloat w, h;
  
  switch(c->data->kind) {
  case Collision2D_circle:
    c->shape = cpCircleShapeNew(body, c->data->radius, cpBodyWorldToLocal(body, cpv(x, y)));
    break;
  case Collision2D_box:
    w = c->data->width / 2;
    h = c->data->height / 2;
    box[0] = cpv(x - w, y - h);
    box[1] = cpv(x + w, y - h);
    box[2] = cpv(x + w, y + h);
    box[3] = cpv(x - w, y + h);
    c->shape = cpBoxShapeNew(body, c->data->width, c->data->height, c->data->radius);
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

static void _create_shapes(void) {
  QUERY(
      ( write, Collision2D, c )
    , ( filter, exclude, Transform2D )
    , ( filter, exclude, Body2D )
  ) {
    if(c->shape != NULL) {
      return;
    }

    struct Body2D * b = Body2D_write(c->body);

    if(b == NULL || b->body == NULL) {
      printf("body not found for collision\n");
      return;
    }

    cpVect xy = cpBodyGetPosition(b->body);
    cpFloat a = cpBodyGetAngle(b->body);

    _new_shape(b->body, c, xy.x, xy.y, a);
  }

  /*
  QUERY(
      ( write, Collision2D, c )
    , ( read, Transform2D, t )
    , ( filter, exclude, Body2D )
  ) {
    if(c->shape != NULL) {
      return;
    }

    struct Body2D * b = Body2D_write(c->body);

    if(b == NULL || b->body == NULL) {
      printf("body not found for collision\n");
      return;
    }

    _new_shape(b->body, c, t->x, t->y, t->a);
  }

  QUERY(
      ( write, Collision2D, c )
    , ( write, Body2D, b )
    , ( filter, exclude, Transform2D )
  ) {
    if(c->shape != NULL) {
      return;
    }

    if(c->body != 0) {
      b = Body2D_write(c->body);
    }

    if(b == NULL || b->body == NULL) {
      printf("body not found for collision\n");
      return;
    }

    cpVect xy = cpBodyGetPosition(b->body);
    cpFloat a = cpBodyGetAngle(b->body);

    _new_shape(b->body, c, xy.x, xy.y, a);
  }

  QUERY(
      ( write, Collision2D, c )
    , ( write, Body2D, b )
    , ( read, Transform2D, t )
  ) {
    if(c->shape != NULL) {
      return;
    }

    if(c->body != 0) {
      b = Body2D_write(c->body);
    }

    if(b == NULL || b->body == NULL) {
      printf("body not found for collision\n");
      return;
    }

    _new_shape(b->body, c, t->x, t->y, t->a);
  }
  */
}

static void _create_constraints(void) {
  QUERY(
      ( write, Constraint2D, c )
  ) {
    if(c->constraint != NULL) {
      if(c->inactive) {
        cpSpaceRemoveConstraint(physics_space(), c->constraint);
        cpConstraintFree(c->constraint);
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

static void _add_events(void) {
  QUERY_EVENT(( read, AddImpulse2D, a )) {
    struct Body2D * body = Body2D_write(a->body);
    if(body && body->body) {
      cpBodyApplyImpulseAtLocalPoint(body->body, cpv(a->impulse[0], a->impulse[1]), cpv(a->point[0], a->point[1]));
    }
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

void physics_update(void) {
  _create_bodies();
  _create_shapes();
  _create_constraints();
  _prepare_kinematic();
  _add_events();
  _iterate();
  _update_transform();
}

