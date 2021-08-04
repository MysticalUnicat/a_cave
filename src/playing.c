#include "input.c"

#include "paused.c"

void _playing_begin(void * ud) {
}

void _playing_frame(void * ud) {
  if(Input.pause) {
    state_push(&paused_state);
    return;
  }
}

struct State playing_state = {
  .begin = _playing_begin,
  .frame = _playing_frame
};

