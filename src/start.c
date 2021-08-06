#pragma once

// #include "playing.c"

void _start_begin(void * ud) {
  Engine_set_physics_speed(alias_R_ONE);
  Engine_pop_state();
  // Engine_push_state(&playing_state);
}

struct State start_state = {
  .begin = _start_begin
};
