#include "transform.h"

extern alias_ecs_Instance * g_world;
alias_TransformBundle g_transform_bundle;

void transform_init(void) {
  alias_TransformBundle_initialize(g_world, &g_transform_bundle);
}

void transform_update(void) {
  // alias_transform_update2d_serial(g_world, &g_transform_bundle);
}

