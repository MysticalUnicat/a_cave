#pragma once

#include "engine/engine.h"

DECLARE_COMPONENT(Movement, {
  enum MovementTarget {
    MovementTarget_None,
    MovementTarget_Point,
    MovementTarget_LocalDirection,
    MovementTarget_WorldDirection,
    MovementTarget_Entity
  } target;
  union {
    alias_ecs_EntityHandle target_entity;
    alias_pga2d_Point      target_point;
    alias_pga2d_Point      target_direction;
  };

  alias_R movement_speed;
  bool done;
})

struct PlayerInputs {
  struct InputSignal pause;
  struct InputSignal left;
  struct InputSignal right;
  struct InputSignal up;
  struct InputSignal down;
  struct InputSignal mouse_position;
  struct InputSignal mouse_left_up;
  struct InputSignal mouse_left_down;
  struct InputSignal mouse_right_up;
  struct InputSignal mouse_right_down;
};

DECLARE_COMPONENT(PlayerControlMovement, {
  uint32_t player_index;
  uint32_t input_index;
  struct PlayerInputs * inputs;
})
