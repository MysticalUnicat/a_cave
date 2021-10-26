#pragma once

#include <stdlib.h>
#include <alias/ecs.h>

#include "paused.c"

DECLARE_COMPONENT(Movement, {
  enum MovementTarget {
    MovementTarget_None,
    MovementTarget_Point,
    MovementTarget_Entity
  } target;
  alias_pga2d_Point      point_target;
  alias_ecs_EntityHandle entity_target;

  alias_R movement_speed;
})
DEFINE_COMPONENT(Movement);

struct {
  struct InputSignalUp pause;
  struct InputSignalPass player_left;
  struct InputSignalPass player_right;
  struct InputSignalPass player_up;
  struct InputSignalPass player_down;
  uint32_t input;

  Entity camera;
  Entity player;
  alias_pga2d_Direction player_target;

  alias_ecs_LayerHandle level_layer;
} _playing = {
  .pause = INPUT_SIGNAL_UP(Binding_Pause),
  .player_left = INPUT_SIGNAL_PASS(Binding_PlayerLeft),
  .player_right = INPUT_SIGNAL_PASS(Binding_PlayerRight),
  .player_up = INPUT_SIGNAL_PASS(Binding_PlayerUp),
  .player_down = INPUT_SIGNAL_PASS(Binding_PlayerDown),
};

union InputSignal * _playing_signals[] = {
  (union InputSignal *)&_playing.pause,
  (union InputSignal *)&_playing.player_left,
  (union InputSignal *)&_playing.player_right,
  (union InputSignal *)&_playing.player_up,
  (union InputSignal *)&_playing.player_down,
};

alias_R alias_random_R(void) {
  return (alias_R)(rand()) / (alias_R)RAND_MAX;
}

void _load_level(void) {
  // make some grass
  for(uint32_t i = 0; i < 100; i++) {
    alias_R x = alias_random_R() * 1000 - 500;
    alias_R y = alias_random_R() * 1000 - 500;

    SPAWN( ( alias_Translation2D, .value = alias_pga2d_direction(x, y) )
         , ( DrawCircle, .radius = 5, .color = alias_Color_from_rgb_u8(100, 255, 100) )
         );
  }
}

void _playing_begin(void * ud) {
  (void)ud;

  _playing.input = Engine_add_input_frontend(0, sizeof(_playing_signals) / sizeof(_playing_signals[0]), _playing_signals);

  _playing.player = SPAWN( ( alias_Transform2D, .value = alias_pga2d_Motor_IDENTITY )
                         , ( alias_Physics2DMotion )
                         , ( alias_Physics2DDampen, .value = 10 )
                         , ( DrawRectangle, .width = 10, .height = 20, .color = alias_Color_from_rgb_u8(100, 100, 255) )
                         );

  _playing.camera = SPAWN( ( alias_Translation2D ) // offset from player
                         , ( alias_Parent2D, .value = _playing.player )
                         , ( Camera, .viewport_max = alias_pga2d_point(alias_R_ONE, alias_R_ONE), .zoom = alias_R_ONE )
                         );

  alias_ecs_create_layer(Engine_ecs(), &(alias_ecs_LayerCreateInfo) {
    .max_entities = 0
  }, &_playing.level_layer);

  _load_level();
}

void _playing_frame(void * ud) {
  (void)ud;

  if(_playing.pause.value) {
    Engine_push_state(&paused_state);
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
}

void _playing_end(void * ud) {
  (void)ud;

  Engine_remove_input_frontend(0, _playing.input);

  alias_ecs_destroy_layer(Engine_ecs(), _playing.level_layer, ALIAS_ECS_LAYER_DESTROY_REMOVE_ENTITIES);
}

struct State playing_state = {
  .begin = _playing_begin,
  .frame = _playing_frame,
  .end   = _playing_end
};
