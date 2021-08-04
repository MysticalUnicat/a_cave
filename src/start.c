#pragma once

#include "playing.c"

void _start_begin(void * ud) {
  g.physics_speed = 1.0f;
  state_pop();
  state_push(&playing_state);
}

struct State start_state = {
  .begin = _start_begin
};
