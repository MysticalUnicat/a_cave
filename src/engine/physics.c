#include "physics.h"

#include <raylib.h>

alias_Physics2DBundle g_physics_bundle;

static float _physics_time = 1.0f;

void physics_init(void) {
  alias_Physics2DBundle_initialize(g_world, &g_physics_bundle, &g_transform_bundle);
}

void physics_transform_update(alias_R speed) {
  const float timestep = 1.0f / 60.0f;
  
  static float p_time = 0.0f;
  static float s_time = 0.0f;

  s_time += GetFrameTime() * speed;

  if(p_time >= s_time) {
    alias_transform_update2d_serial(g_world, &g_transform_bundle);
  }

  while(p_time < s_time) {
    alias_physics_update2d_serial_pre_transform(g_world, &g_physics_bundle, timestep);

    alias_transform_update2d_serial(g_world, &g_transform_bundle);

    alias_physics_update2d_serial_post_transform(g_world, &g_physics_bundle, timestep);

    p_time += timestep;
  }
}

