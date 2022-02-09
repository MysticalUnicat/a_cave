#include "../component.h"

alias_ecs_EntityHandle _spawn_player(alias_ecs_LayerHandle layer, alias_pga2d_Point origin, Entity target) {
  return SPAWN( ( alias_Transform2D, .value = alias_pga2d_translator_to(origin) )
              , ( PlayerControlMovement, .player_index = 0, .target = target )
              , ( alias_Physics2DDampen, .value = 5 )
              , ( Armor
                , .live = LIVE_VALUE(100)
                , .max = GAME_VALUE(100)
                , .regen_per_second = GAME_VALUE(0)
                , .regen_percent_per_second = GAME_VALUE(0)
                )
              , ( Shield
                , .live = LIVE_VALUE(100)
                , .max = GAME_VALUE(100)
                , .regen_per_second = GAME_VALUE(0)
                , .regen_percent_per_second = GAME_VALUE(0)
                , .recharge_delay = GAME_VALUE(3)
                , .recharge_per_second = GAME_VALUE(0)
                , .recharge_percentage_per_second = GAME_VALUE(20)
                , .recharge_power_per_second = GAME_VALUE(50)
                )
              , ( Power
                , .live = LIVE_VALUE(0)
                , .max  = GAME_VALUE(100)
                , .generation_per_second = GAME_VALUE(40)
                )
              , ( DrawRectangle, .width = 20, .height = 10, .color = alias_Color_from_rgb_u8(100, 100, 255) )
              , ( DrawCircle, .radius = 6, .color = alias_Color_from_rgb_u8(100, 100, 255) )
              );
}
