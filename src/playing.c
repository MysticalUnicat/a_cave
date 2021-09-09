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
  alias_Point2D          point_target;
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
  alias_Vector2D player_target;

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

    SPAWN( ( alias_Translation2D, .value.x = x, .value.y = y )
         , ( DrawCircle, .radius = 5, .color = alias_Color_from_rgb_u8(100, 255, 100) )
         );
  }
}

void _playing_begin(void * ud) {
  (void)ud;

  _playing.input = Engine_add_input_frontend(0, sizeof(_playing_signals) / sizeof(_playing_signals[0]), _playing_signals);

  _playing.player = SPAWN( ( alias_Translation2D, .value.x = 0, .value.y = 0 )
                         , ( alias_Physics2DLinearMotion, .velocity.x = 0, .velocity.y = 0, .damping = 0.9f )
                         , ( DrawCircle, .radius = 10, .color = alias_Color_from_rgb_u8(100, 100, 255) )
                         );

  _playing.camera = SPAWN( ( alias_Translation2D, .value.x = 0, .value.y = 0 ) // offset from player
                         , ( alias_Parent2D, .value = _playing.player )
                         , ( Camera, .viewport_max = { alias_R_ONE, alias_R_ONE }, .zoom = alias_R_ONE )
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

  alias_R move_speed = 50.0f;

  alias_Vector2D velocity = {
      .x = _playing.player_right.value - _playing.player_left.value
    , .y = _playing.player_down.value - _playing.player_up.value
  };

  alias_Physics2DLinearMotion * limotion;
  alias_ecs_write_entity_component(Engine_ecs(), _playing.player, alias_Physics2DLinearMotion_component(), (void **)&limotion);

  alias_R vlength = alias_Vector2D_length(velocity);
  if(!alias_R_is_zero(vlength)) {
    limotion->velocity = alias_multiply_Vector2D_R(velocity, move_speed / vlength);
  }
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
