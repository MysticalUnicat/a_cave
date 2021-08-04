#pragma once

void _paused_begin(void * ud) {
  g.physics_speed = 0.0f;
}

void _paused_frame(void * ud) {
  extern struct State playing;

  if(IsKeyPressed('P')) {
    state_pop();
    return;
  }

  DrawText("GAME PAUSED", SCREEN_WIDTH / 2 - MeasureText("GAME PAUSED", 40) / 2, SCREEN_HEIGHT/2 - 40, 40, GRAY);
}

void _paused_end(void * ud) {
  g.physics_speed = 1.0f;
}

struct State paused_state = {
  .begin = _paused_begin,
  .frame = _paused_frame,
  .end = _paused_end
};
