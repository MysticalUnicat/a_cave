#include "../component.h"

alias_ecs_EntityHandle _spawn_player(alias_ecs_LayerHandle layer, alias_pga2d_Point origin) {
  return SPAWN( ( alias_Transform2D, .value = alias_pga2d_translator_to(origin) )
              , ( PlayerControlMovement, .player_index = 0 )
              , ( alias_Physics2DDampen, .value = 10 )
              , ( DrawRectangle, .width = 10, .height = 20, .color = alias_Color_from_rgb_u8(100, 100, 255) )
              );
}
