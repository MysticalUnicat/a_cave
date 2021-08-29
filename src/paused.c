#pragma once

#include "engine/engine.h"

struct {
  struct InputSignalDown unpause;
  uint32_t input;
} _paused = {
  .unpause = INPUT_SIGNAL_DOWN(Binding_Pause)
};

union InputSignal * _paused_signals[] = {
  (union InputSignal *)&_paused.unpause
};

void _paused_begin(void * ud) {
  (void)ud;

  Engine_set_physics_speed(0.01);
}

void _paused_frame(void * ud) {
  (void)ud;

  if(_paused.unpause.value || menu_back.value) {
    Engine_pop_state();
    return;
  }

  Engine_ui_center();
    Engine_ui_font_size(40);
    Engine_ui_font_color(Color_GRAY);
    Engine_ui_text("GAME PAUSED");
}

void _paused_end(void * ud) {
  (void)ud;

  Engine_set_physics_speed(alias_R_ONE);
}

struct State paused_state = {
  .begin = _paused_begin,
  .frame = _paused_frame,
  .end = _paused_end
};
