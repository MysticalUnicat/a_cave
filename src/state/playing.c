#include <stdlib.h>
#include <alias/ecs.h>

#include "local.h"

// system
extern void player_movement_system(void);
extern void movement_system(void);

// prefab
extern alias_ecs_EntityHandle _spawn_grass(alias_ecs_LayerHandle layer, alias_pga2d_Point origin);
extern alias_ecs_EntityHandle _spawn_player(alias_ecs_LayerHandle layer, alias_pga2d_Point origin);
extern alias_ecs_EntityHandle _spawn_camera(alias_ecs_LayerHandle layer, alias_ecs_EntityHandle target);

extern struct State paused_state;

struct {
  Entity camera;
  Entity player;
  alias_pga2d_Direction player_target;

  alias_ecs_LayerHandle player_layer;
  alias_ecs_LayerHandle level_layer;
} _playing;

alias_R alias_random_R(void) {
  return (alias_R)(rand()) / (alias_R)RAND_MAX;
}

static void _shoot_bullet(alias_ecs_EntityHandle entity, void * * write_data, void * * read_data) {
}

void _load_level(void) {
  // make some grass
  for(uint32_t i = 0; i < 100; i++) {
    alias_R x = alias_random_R() * 1000 - 500;
    alias_R y = alias_random_R() * 1000 - 500;

    _spawn_grass(_playing.level_layer, alias_pga2d_point(x, y));
  }
}

void _playing_begin(void * ud) {
  (void)ud;

  alias_ecs_create_layer(Engine_ecs(), &(alias_ecs_LayerCreateInfo) {
    .max_entities = 0
  }, &_playing.player_layer);

  _playing.player = _spawn_player(_playing.player_layer, alias_pga2d_point(0, 0));
  _playing.camera = _spawn_camera(_playing.player_layer, _playing.player);

  alias_ecs_create_layer(Engine_ecs(), &(alias_ecs_LayerCreateInfo) {
    .max_entities = 0
  }, &_playing.level_layer);

  _load_level();
}

void _playing_frame(void * ud) {
  (void)ud;
  player_movement_system();
  movement_system();
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
