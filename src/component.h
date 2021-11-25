#pragma once

#include "engine/engine.h"

DECLARE_COMPONENT(Movement, {
  enum MovementTarget {
    MovementTarget_None,
    MovementTarget_Point,
    MovementTarget_Direction,
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
};

DECLARE_COMPONENT(PlayerControlMovement, {
  uint32_t player_index;
  uint32_t input_index;
  struct PlayerInputs * inputs;
})

