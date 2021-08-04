#pragma once

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
