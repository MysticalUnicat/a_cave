#include "physics.h"

/*
extern alias_ecs_Instance * World();

DEFINE_COMPONENT(World(), Transform2D)

DEFINE_COMPONENT(World(), Body2D)

DEFINE_COMPONENT(World(), BoxCollision2D)

DEFINE_COMPONENT(World(), CircleCollision2D)

cpSpace * _physics_space;

void physics_create_new_bodies(void);
void physics_iterate(void);
void physics_update_transforms(void);

void physics_init(void) {
  _physics_space = cpSpaceNew();
  cpSpaceSetGravity(space, cpv(0, 0));
}

void physics_frame(void) {
  physics_create_new_bodies();

  physics_iterate();

  physics_update_transforms();
}

void physics_create_new_bodies(void) {
  // TODO detect when Body2D was added
  QUERY(
      World()
    , ( read, Transform2D, t )
    , ( read, BoxCollision2D, c )
    , ( write, Body2D, b )
  ) {
    if(b->body != NULL) {
      return;
    }

    switch(b->kind) {
    case Body2D_dynamic:
      if(b->mass < 0.0001f) {
        b->mass = (c->r - c->l) * (c->b - c->t);
      }

      if(b->moment < 0.0001f) {
        b->moment = cpMomentForBox(b->mass, (c->r - c->l), (c->b - c->t));
      }

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
  }
}

void physics_iterate(void) {
  static float accum = 0.0f;
  const float timestep = 1.0f / 60.0f;
  
  float time = GetTime();

  while(accum < time) {
    cpSpaceStep(_physics_space, timestep);
    accum += timestep;
  }
}
  
void physics_update_transforms(void) {
  QUERY(
      World()
    , ( write, Transform2D, t )
    , ( read, Body2D, b )
  ) {
    if(by->body == NULL) {
      return;
    }

    cpVect p = cpBodyGetPosition(b->body);
    t->x = p.x;
    t->y = p.y;
    t->a = cpBodyGetAngle(b->body);
  }
}
*/
