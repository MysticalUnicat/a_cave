#pragma once

#include <stddef.h>

struct State {
  struct State * prev;
  void * ud;
  void (*begin)(void * ud);
  void (*frame)(void * ud);
  void (*background)(void * ud);
  void (*pause)(void * ud);
  void (*unpause)(void * ud);
  void (*end)(void * ud);
};

void state_push(struct State * state);
void state_pop(void);
void state_frame(void);

