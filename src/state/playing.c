#include <stdlib.h>
#include <alias/ecs.h>
#include <alias/random.h>

#include "local.h"

// system
extern void player_movement_system(void);
extern void movement_system(void);
extern void armor_system(void);
extern void shield_system(void);
extern void power_system(void);

// prefab
extern alias_ecs_EntityHandle _spawn_grass(alias_ecs_LayerHandle layer, alias_pga2d_Point origin);
extern alias_ecs_EntityHandle _spawn_target(alias_ecs_LayerHandle layer);
extern alias_ecs_EntityHandle _spawn_player(alias_ecs_LayerHandle layer, alias_pga2d_Point origin, alias_ecs_EntityHandle target);
extern alias_ecs_EntityHandle _spawn_camera(alias_ecs_LayerHandle layer, alias_ecs_EntityHandle target);

extern struct State paused_state;

struct {
  Entity target;
  Entity camera;
  Entity player;
  alias_pga2d_Direction player_target;

  alias_ecs_LayerHandle player_layer;
  alias_ecs_LayerHandle level_layer;
} _playing;

// plan:
// - call functions at a certain time
//   - setting up the enemy types that can spawn
//   - (de)spawn certain enemys in a pattern
// - 

void _load_level(void) {
  // make some grass
  for(uint32_t i = 0; i < 1000; i++) {
    alias_R x = alias_random_f32_snorm() * 100;
    alias_R y = alias_random_f32_snorm() * 100;

    _spawn_grass(_playing.level_layer, alias_pga2d_point(x, y));
  }
}

void _playing_begin(void * ud) {
  (void)ud;

  Engine_physics_2d_bundle()->gravity = (alias_pga2d_Direction) { .e02 = 9.81 };

  alias_ecs_create_layer(Engine_ecs(), &(alias_ecs_LayerCreateInfo) {
    .max_entities = 0
  }, &_playing.player_layer);

  _playing.target = _spawn_target(_playing.player_layer);
  _playing.player = _spawn_player(_playing.player_layer, alias_pga2d_point(0, 0), _playing.target);
  _playing.camera = _spawn_camera(_playing.player_layer, _playing.player);
  PlayerControlMovement_write(_playing.player)->inputs->mouse_position.click_camera = _playing.camera;

  alias_ecs_create_layer(Engine_ecs(), &(alias_ecs_LayerCreateInfo) {
    .max_entities = 0
  }, &_playing.level_layer);

  _load_level();
}

void _playing_frame(void * ud) {
  (void)ud;
  player_movement_system();
  movement_system();
  armor_system();
  shield_system();
  power_system();

  Engine_ui_bottom_left();
  Engine_ui_vertical(); {
    Engine_ui_font_size(18);
    Engine_ui_font_color(alias_Color_BLACK);
    
    const struct Armor * armor = Armor_read(_playing.player);
    if(armor != NULL) {
      Engine_ui_text("ARMOR: %'.2f / %'.2f", armor->live.current, GameValue_get(&armor->max));
    }

    const struct Shield * shield = Shield_read(_playing.player);
    if(shield != NULL) {
      Engine_ui_text("SHIELD: %'.2f / %'.2f", shield->live.current, GameValue_get(&shield->max));
    }

    const struct Power * power = Power_read(_playing.player);
    if(power != NULL) {
      Engine_ui_text("POWER: %'.2f / %'.2f", power->live.current, GameValue_get(&power->max));
    }
  } Engine_ui_end();
}

void _playing_end(void * ud) {
  (void)ud;

  alias_ecs_destroy_layer(Engine_ecs(), _playing.player_layer, ALIAS_ECS_LAYER_DESTROY_REMOVE_ENTITIES);
  alias_ecs_destroy_layer(Engine_ecs(), _playing.level_layer, ALIAS_ECS_LAYER_DESTROY_REMOVE_ENTITIES);
}

struct State playing_state = {
  .begin = _playing_begin,
  .frame = _playing_frame,
  .end   = _playing_end
};
