#include "../component.h"

alias_ecs_EntityHandle _spawn_target(alias_ecs_LayerHandle layer) {
  return SPAWN( ( alias_Translation2D )
              , ( DrawCircle, .radius = 6, .color = alias_Color_from_rgb_u8(255, 100, 100) )
              );
}
