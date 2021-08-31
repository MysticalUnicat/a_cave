#pragma once

#include "paused.c"

struct {
  struct InputSignalUp pause;
  uint32_t input;

  Entity camera;
  Entity player;
} _playing = {
  .pause = INPUT_SIGNAL_UP(Binding_Pause)
};

union InputSignal * _playing_signals[] = {
  (union InputSignal *)&_playing.pause
};

void _playing_begin(void * ud) {
  (void)ud;

  _playing.input = Engine_add_input_frontend(0, sizeof(_playing_signals) / sizeof(_playing_signals[0]), _playing_signals);

  _playing.player = SPAWN( ( alias_Translation2D, .value.x = 0, .value.y = 0 )
                         , ( DrawCircle, .radius = 5, .color = alias_Color_from_rgb_u8(100, 100, 255) )
                         );

  _playing.camera = SPAWN( ( alias_Parent2D, .value = _playing.player )
                         , ( Camera, .viewport_max = { alias_R_ONE, alias_R_ONE }, .zoom = alias_R_ONE )
                         );
}

void _playing_frame(void * ud) {
  (void)ud;

  if(_playing.pause.value) {
    Engine_push_state(&paused_state);
  }
}

void _playing_end(void * ud) {
  (void)ud;

  Engine_remove_input_frontend(0, _playing.input);
}

struct State playing_state = {
  .begin = _playing_begin,
  .frame = _playing_frame,
  .end   = _playing_end
};
