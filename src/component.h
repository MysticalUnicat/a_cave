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

typedef struct LiveValue {
  alias_R current;
  alias_R damage;

  alias_R depleted_time;
  alias_R last_damage_time;
  alias_R full_time;
} LiveValue;

#define LIVE_VALUE(X) (LiveValue) { .current = X, .damage = alias_R_ZERO }

static inline void LiveValue_update(LiveValue * live, alias_R delta, alias_R max) {
  delta *= Engine_frame_time();
  if(live->damage > alias_R_ZERO) {
    live->last_damage_time = Engine_time();
    delta -= live->damage;
  }
  live->damage = alias_R_ZERO;
  bool was_depleted = live->current <= alias_R_ZERO;
  bool was_full = live->current >= max;
  live->current += delta;
  bool is_depleted = live->current <= alias_R_ZERO;
  bool is_full = live->current >= max;
  if(is_depleted) {
    live->current = alias_R_ZERO;
    if(!was_depleted) {
      live->depleted_time = Engine_time();
    }
  }
  if(is_full) {
    live->current = max;
    if(!was_full) {
      live->full_time = Engine_time();
    }
  }
}

typedef struct GameValue {
  alias_R current;
  alias_R base;
  alias_R added;
  alias_R increased;
  alias_R more;
} GameValue;

#define GAME_VALUE(X) (GameValue) { .current = alias_R_NAN, .base = X, .added = alias_R_ZERO, .increased = alias_R_ONE, .more = alias_R_ONE }

static inline void GameValue_init(GameValue * gv, alias_R base) {
  gv->base = base;
  gv->added = alias_R_ZERO;
  gv->increased = alias_R_ONE;
  gv->more = alias_R_ONE;
}

// amount must be greater than 0
static inline void GameValue_added(GameValue * gv, alias_R amount) {
  gv->current = alias_R_NAN;
  gv->added += amount;
}

// amount must be greater than 0
static inline void GameValue_subtracted(GameValue * gv, alias_R amount) {
  gv->current = alias_R_NAN;
  gv->added -= amount;
}

// amount must be greater than 0, 1 is the same as 100% increased
static inline void GameValue_increase(GameValue * gv, alias_R amount) {
  gv->current = alias_R_NAN;
  gv->increased += amount;
}

// amount must be greater than 0, 1 is the same as 100% decreased
static inline void GameValue_decrease(GameValue * gv, alias_R amount) {
  gv->current = alias_R_NAN;
  gv->increased -= amount;
}

// amount must be greater than 0, 1 is the same as 100% more
static inline void GameValue_more(GameValue * gv, alias_R amount) {
  gv->current = alias_R_NAN;
  gv->more *= alias_R_ONE + amount;
}

// amount must be greater than 0, 1 is the same as 100% less
static inline void GameValue_less(GameValue * gv, alias_R amount) {
  gv->current = alias_R_NAN;
  gv->more /= alias_R_ONE + amount;
}

static inline alias_R GameValue_get(const GameValue * gv) {
  if(alias_R_isnan(gv->current)) {
    *(alias_R *)&gv->current = (gv->base + gv->added) * gv->increased * gv->more;
  }
  return gv->current;
}

DECLARE_COMPONENT(Power, {
  LiveValue live;
  GameValue max;

  GameValue generation_per_second;
})

DECLARE_COMPONENT(Armor, {
  LiveValue live;
  GameValue max;

  GameValue regen_per_second;
  GameValue regen_percent_per_second;
})

DECLARE_COMPONENT(Shield, {
  LiveValue live;
  bool recharging;
  GameValue max;

  GameValue regen_per_second;
  GameValue regen_percent_per_second;

  GameValue recharge_delay;
  GameValue recharge_per_second;
  GameValue recharge_percentage_per_second;
  GameValue recharge_power_per_second;
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
  Entity target;
  struct PlayerInputs * inputs;
})
