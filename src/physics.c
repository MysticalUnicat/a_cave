#include "physics.h"
#include "event.h"

DEFINE_COMPONENT(Body2D)

DEFINE_COMPONENT(Collision2D)

DEFINE_COMPONENT(Constraint2D)

DEFINE_COMPONENT(AddImpulse2D)

DEFINE_COMPONENT(Contact2D)

static inline cpSpace * _create_space(void) {
  cpSpace * space = cpSpaceNew();
  cpCollisionHandler * handler = cpSpaceAddDefaultCollisionHandler(space);
  return space;
}

LAZY_GLOBAL(cpSpace *, physics_space, inner = _create_space();)

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

    cpBodySetUserData(b->body, (void *)entity);

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

    cpBodySetUserData(b->body, (void *)entity);
    cpBodySetPosition(b->body, (cpVect) { t->position.x, t->position.y });

    cpSpaceAddBody(physics_space(), b->body);
  }
}

static void _new_shape(Entity entity, struct Body2D * b, struct Collision2D * c, const struct Transform2D * t) {
  cpVect body_xy = cpBodyGetPosition(b->body);
  float
      body_x = body_xy.x
    , body_y = body_xy.y
    , body_a = cpBodyGetAngle(b->body)
    ;

  switch(c->kind) {
  case Collision2D_circle:
    {
      cpVect local_position = cpBodyWorldToLocal(b->body, cpv(t->x, t->y));
      c->shape = cpCircleShapeNew(b->body, c->radius, local_position);
    }
    break;
  case Collision2D_box:
    {
      float
          hw = c->width / 2
        , hh = c->height / 2
        , bl = -hw
        , br =  hw
        , bt = -hh
        , bb =  hh
        ;

      point2 box[] = {
          { br, bb }
        , { br, bt }
        , { bl, bt }
        , { bl, bb }
        };
      cpVect verts[4];

      // the verts need to be in local space of the body
      matrix23 body_inverse_matrix = create_matrix23_inverse(body_x, body_y, body_a);
      matrix23 shape_matrix = create_matrix23(t->x, t->y, t->a);
      matrix23 m = multiply_matrix23_matrix23(shape_matrix, body_inverse_matrix);

      for(uint32_t i = 0; i < 4; i++) {
        point2 p = multiply_matrix23_point2(m, box[i]);
        verts[i].x = p.x;
        verts[i].y = p.y;
      }

      c->shape = cpPolyShapeNewRaw(b->body, 4, verts, 1.0);
    }
    break;
  }

  if(c->shape == NULL) {
    return;
  }

  cpShapeSetUserData(c->shape, (void *)entity);
  cpShapeSetSensor(c->shape, c->sensor);
  cpShapeSetElasticity(c->shape, c->elasticity);
  cpShapeSetFriction(c->shape, c->friction);
  cpShapeSetCollisionType(c->shape, c->collision_type);

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
      return;
    }
    
    _new_shape(entity, b, c, &Transform2D_zero);
  }

  QUERY(
      ( write, Collision2D, c )
    , ( read, Transform2D, t )
    , ( filter, exclude, Body2D )
  ) {
    if(c->shape != NULL) {
      if(c->inactive) {
        cpSpaceRemoveShape(physics_space(), c->shape);
        cpShapeFree(c->shape);
        
      }
      return;
    }
    struct Body2D * b = Body2D_write(c->body);
    if(b == NULL || b->body == NULL) {
      return;
    }
    _new_shape(entity, b, c, t);
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
      return;
    }
    _new_shape(entity, b, c, &Transform2D_zero);
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
      return;
    }
    _new_shape(entity, b, c, t);
  }
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
      c->constraint = cpPinJointNew(a, b, cpv(c->anchor_a.x, c->anchor_a.y), cpv(c->anchor_b.x, c->anchor_b.y));
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

    cpBodySetVelocity(b->body, cpv(v->velocity.x, v->velocity.y));
    cpBodySetAngularVelocity(b->body, v->angular_velocity);

    v->velocity.x = 0.0f;
    v->velocity.y = 0.0f;
    v->angular_velocity = 0.0f;
  }
}

static void _add_events(void) {
  QUERY_EVENT(( read, AddImpulse2D, a )) {
    struct Body2D * body = Body2D_write(a->body);
    if(body && body->body) {
      cpBodyApplyImpulseAtLocalPoint(body->body, cpv(a->impulse.x, a->impulse.y), cpv(a->point.x, a->point.y));
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
    t->position.x = p.x;
    t->position.y = p.y;
    t->angle = cpBodyGetAngle(b->body);
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

