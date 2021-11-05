#include "../component.h"

static struct Image img_uncut = { "grass.png" };

alias_ecs_EntityHandle _spawn_grass(alias_ecs_LayerHandle layer, alias_pga2d_Point origin) {
  origin.e12 = alias_R_ZERO;
  return SPAWN_LAYER(
      layer
    , ( alias_Translation2D, .value = origin )
    , ( Sprite, .image = &img_uncut, .s1 = 1, .t1 = 1, .color = alias_Color_from_rgb_u8(255, 255, 255) )
    );
}
