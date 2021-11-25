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
  //struct InputSignalUp pause;
  //struct InputSignalPass player_left;
  //struct InputSignalPass player_right;
  //struct InputSignalPass player_up;
  //struct InputSignalPass player_down;
  //uint32_t input;

  Entity camera;
  Entity player;
  alias_pga2d_Direction player_target;

  alias_ecs_LayerHandle player_layer;
  alias_ecs_LayerHandle level_layer;
} _playing = {
  //.pause = INPUT_SIGNAL_UP(Binding_Pause),
  //.player_left = INPUT_SIGNAL_PASS(Binding_PlayerLeft),
  //.player_right = INPUT_SIGNAL_PASS(Binding_PlayerRight),
  //.player_up = INPUT_SIGNAL_PASS(Binding_PlayerUp),
  //.player_down = INPUT_SIGNAL_PASS(Binding_PlayerDown),
};

//union InputSignal * _playing_signals[] = {
//  (union InputSignal *)&_playing.pause,
//  (union InputSignal *)&_playing.player_left,
//  (union InputSignal *)&_playing.player_right,
//  (union InputSignal *)&_playing.player_up,
//  (union InputSignal *)&_playing.player_down,
//};

alias_R alias_random_R(void) {
  return (alias_R)(rand()) / (alias_R)RAND_MAX;
}

static void _shoot_bullet(alias_ecs_EntityHandle entity, void * * write_data, void * * read_data) {
  /*
  const alias_Transform2D * transform = (const alias_Transform2D *)read_data[0];
  SPAWN( ( alias_Transform2D, .value = { .one = 1, .e02 = transform->value.e02, .e01 = transform->value.e01 } )
       , ( alias_Physics2DMotion, .value = alias_pga2d_translator(1,
  */
}

void _load_level(void) {
  // make some grass
  for(uint32_t i = 0; i < 100; i++) {
    alias_R x = alias_random_R() * 1000 - 500;
    alias_R y = alias_random_R() * 1000 - 500;

    _spawn_grass(_playing.level_layer, alias_pga2d_point(x, y));
  }

  // make a bullet shooter
  /*
  SPAWN( ( alias_Translation2D, .value = alias_pga2d_direction(0, 5) )
       , ( DrawCircle, .radius = 10, .color = alias_Color_from_rgb_u8(127, 0, 0) )
       , ( Timer, .period = 500, .function = _shoot_bullet, .num_read_components = 1, .read_components = (alias_ecs_ComponentHandle[]) { alias_Transform2d_component() } )
       );
  */
}

void _playing_begin(void * ud) {
  (void)ud;

  //_playing.input = Engine_add_input_frontend(0, sizeof(_playing_signals) / sizeof(_playing_signals[0]), _playing_signals);

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

  #if 0
  if(_playing.pause.boolean) {
    Engine_push_state(&paused_state);
    return;
  }

  alias_R move_speed = 1000.0f;

  alias_R dir_x = _playing.player_left.value - _playing.player_right.value;
  alias_R dir_y = _playing.player_down.value - _playing.player_up.value;

  // link event to forque pushers?
  alias_Transform2D * transform;
  alias_Physics2DBodyMotion * body;
  alias_ecs_write_entity_component(Engine_ecs(), _playing.player, alias_Transform2D_component(), (void **)&transform);
  alias_ecs_write_entity_component(Engine_ecs(), _playing.player, alias_Physics2DBodyMotion_component(), (void **)&body);

  alias_pga2d_Motor position = transform->value;

  #if 0
  alias_pga2d_Point center = alias_pga2d_sandwich_bm(alias_pga2d_point(0, 0), transform->value);
  alias_pga2d_Point offset = alias_pga2d_add_bb(center, alias_pga2d_direction(dir_x, dir_y));
  alias_pga2d_Line force_line = alias_pga2d_join_bb(center, offset);

  body->forque = alias_pga2d_add(
      alias_pga2d_v(body->forque),
      alias_pga2d_mul(alias_pga2d_s(move_speed), alias_pga2d_v(force_line)));
  #else
  // fake force line
  alias_pga2d_Bivector force_line = (alias_pga2d_Bivector){ .e01 = dir_x, .e02 = dir_y };

  body->forque = alias_pga2d_add(
      alias_pga2d_v(body->forque),
      alias_pga2d_mul(alias_pga2d_s(move_speed),
                      alias_pga2d_dual(alias_pga2d_grade_2(alias_pga2d_sandwich(
                          alias_pga2d_b(force_line),
                          alias_pga2d_reverse_m(position))))));
  #endif

  //printf("playing, velocity = %g %g %g\n", limotion->value.e02, limotion->value.e01, limotion->value.e12);
  #else
  player_movement_system();
  movement_system();
  #endif
}

void _playing_end(void * ud) {
  (void)ud;

  //Engine_remove_input_frontend(0, _playing.input);

  alias_ecs_destroy_layer(Engine_ecs(), _playing.player_layer, ALIAS_ECS_LAYER_DESTROY_REMOVE_ENTITIES);
  alias_ecs_destroy_layer(Engine_ecs(), _playing.level_layer, ALIAS_ECS_LAYER_DESTROY_REMOVE_ENTITIES);
}

struct State playing_state = {
  .begin = _playing_begin,
  .frame = _playing_frame,
  .end   = _playing_end
};
