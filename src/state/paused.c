#include "local.h"

struct {
  struct InputSignalUp unpause;
  uint32_t input;
} _paused = {
  .unpause = INPUT_SIGNAL_UP(Binding_Pause)
};

union InputSignal * _paused_signals[] = {
  (union InputSignal *)&_paused.unpause
};

void _paused_begin(void * ud) {
  (void)ud;

  _paused.input = Engine_add_input_frontend(0, sizeof(_paused_signals) / sizeof(_paused_signals[0]), _paused_signals);

  Engine_set_physics_speed(alias_R_ZERO);
}

void _paused_frame(void * ud) {
  (void)ud;

  if(_paused.unpause.value || menu_back.value) {
    printf("UNPAUSE\n");
    Engine_pop_state();
    return;
  }

  Engine_ui_center();
    Engine_ui_font_size(40);
    Engine_ui_font_color(alias_Color_GRAY);
    Engine_ui_text("GAME PAUSED");
}

void _paused_end(void * ud) {
  (void)ud;

  Engine_remove_input_frontend(0, _paused.input);

  Engine_set_physics_speed(alias_R_ONE);
}

struct State paused_state = {
  .begin = _paused_begin,
  .frame = _paused_frame,
  .end = _paused_end
};
