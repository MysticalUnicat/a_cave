#pragma once

#include <alias/transform.h>

extern alias_TransformBundle g_transform_bundle;
extern alias_ecs_Instance * g_world;

static const alias_LocalToWorld2D alias_LocalToWorld2D_zero = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };

static inline alias_ecs_ComponentHandle alias_Translation2D_component(void) {
  return g_transform_bundle.Translation2D_component;
}

static inline alias_ecs_ComponentHandle alias_Rotation2D_component(void) {
  return g_transform_bundle.Rotation2D_component;
}

static inline alias_Rotation2D * alias_Rotation2D_write(alias_ecs_EntityHandle e) {
  alias_Rotation2D * result;
  alias_ecs_write_entity_component(g_world, e, g_transform_bundle.Rotation2D_component, (void **)&result);
  return result;
}

static inline alias_ecs_ComponentHandle alias_LocalToWorld2D_component(void) {
  return g_transform_bundle.LocalToWorld2D_component;
}

static inline alias_ecs_ComponentHandle alias_Parent2D_component(void) {
  return g_transform_bundle.Parent2D_component;
}

void transform_init(void);
void transform_update(void);

