#include "state.h"

static struct State * _current = NULL;

void state_push(struct State * state) {
  if(_current != NULL && _current->pause != NULL) {
    _current->pause(_current->ud);
  }
  state->prev = _current;
  _current = state;
  if(state->begin != NULL) {
    state->begin(state->ud);
  }
}

void state_pop(void) {
  if(_current == NULL) {
    return;
  }
  if(_current->end != NULL) {
    _current->end(_current->ud);
  }
  _current = _current->prev;
  if(_current->unpause != NULL) {
    _current->unpause(_current->ud);
  }
}

void state_frame(void) {
  struct State * current = _current;
  if(current->frame != NULL) {
    current->frame(current->ud);
  }
  while(current != NULL) {
    if(current->background != NULL) {
      current->background(current->ud);
    }
    current = current->prev;
  }
}

