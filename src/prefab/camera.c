#include "../component.h"

alias_ecs_EntityHandle _spawn_camera(alias_ecs_LayerHandle layer, alias_ecs_EntityHandle target) {
  return SPAWN_LAYER(
      layer
    , ( alias_Translation2D ) // offset from player
    , ( alias_Parent2D, .value = target )
    , ( Camera, .viewport_max = alias_pga2d_point(alias_R_ONE, alias_R_ONE), .zoom = alias_R_ONE )
    );
}
