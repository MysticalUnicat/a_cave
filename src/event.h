#pragma once

#include "util.h"

DECLARE_COMPONENT(Event, {
  uint32_t id;
})

#define SPAWN_EVENT(...) SPAWN(( Event, .id = event_id() ), ## __VA_ARGS__)

uint32_t event_id(void);
void event_update(void);
