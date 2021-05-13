#pragma once

#include "util.h"

DECLARE_COMPONENT(Event, {
  uint32_t id;
})

#define SPAWN_EVENT(...) SPAWN(( Event, .id = event_id() ), ## __VA_ARGS__)

#define EVENT_QUERY(...) \
  static uint32_t CAT(_leid, __LINE__) = 0; \
  uint32_t CAT(_neid, __LINE__) = CAT(_leid, __LINE__); \
  _QUERY( \
    if(_event->id <= CAT(_leid, __LINE__)) return; \
    if(_event->id > CAT(_neid, __LINE__)) CAT(_neid, __LINE__) = _event->id; \
  , \
    CAT(_leid, __LINE__) = CAT(_neid, __LINE__); \
  , ( read, Event, _event ), ## __VA_ARGS__)

uint32_t event_id(void);
void event_update(void);
