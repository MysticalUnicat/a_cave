#include "event.h"

DEFINE_COMPONENT(Event)

static uint32_t _id = 1;

uint32_t event_id(void) {
  return _id++;
}

void event_update(void) {
  static uint32_t last_highest_seen = 0;

  static struct CmdBuf cbuf;
  CmdBuf_begin_recording(&cbuf);

  uint32_t next_highest_seen = last_highest_seen;
  QUERY(( read, Event, e )) {
    if(e->id < last_highest_seen) {
      CmdBuf_despawn(&cbuf, entity);
    } else if(e->id > next_highest_seen) {
      next_highest_seen = e->id;
    }
  }

  last_highest_seen = next_highest_seen;

  CmdBuf_end_recording(&cbuf);
  CmdBuf_execute(&cbuf, g_world);
}
