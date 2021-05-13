#include "util.h"

#include <stdlib.h>
#include <string.h>

struct Cmd * _CmdBuf_allocate(struct CmdBuf * cbuf, size_t size) {
  if(cbuf->pointer + size > cbuf->end) {
    size_t new_capacity = (size + (cbuf->end - cbuf->start));
    new_capacity += new_capacity >> 1;
    void * start = realloc(cbuf->start, new_capacity);
    size_t offset = cbuf->pointer - cbuf->start;
    cbuf->start = start;
    cbuf->pointer = start + offset;
    cbuf->end = start + new_capacity;
  }
  struct Cmd * cmd = (struct Cmd *)cbuf->pointer;
  cmd->size = size;
  cbuf->pointer += size;
  return cmd;
}

void CmdBuf_add_component(struct CmdBuf * cbuf, alias_ecs_EntityHandle entity, alias_ecs_ComponentHandle component, void * data, size_t data_size) {
  struct Cmd * cmd = _CmdBuf_allocate(cbuf, sizeof(*cmd) + data_size);
  cmd->tag = cmd_add_component;
  cmd->entity = entity;
  cmd->component = component;
  memcpy(cmd + 1, data, data_size);
}

void CmdBuf_remove_component(struct CmdBuf * cbuf, alias_ecs_EntityHandle entity, alias_ecs_ComponentHandle component) {
  struct Cmd * cmd = _CmdBuf_allocate(cbuf, sizeof(*cmd));
  cmd->tag = cmd_remove_component;
  cmd->entity = entity;
  cmd->component = component;
}

void CmdBuf_despawn(struct CmdBuf * cbuf, alias_ecs_EntityHandle entity) {
  struct Cmd * cmd = _CmdBuf_allocate(cbuf, sizeof(*cmd));
  cmd->tag = cmd_despawn;
  cmd->entity = entity;
}

void CmdBuf_begin_recording(struct CmdBuf * cbuf) {
  cbuf->pointer = cbuf->start;
}

void CmdBuf_end_recording(struct CmdBuf * cbuf) {
  cbuf->end = cbuf->pointer;
}

void CmdBuf_execute(struct CmdBuf * cbuf, alias_ecs_Instance * instance) {
  struct Cmd * cmd = (struct Cmd *)cbuf->start;
  while((void *)cmd < cbuf->end) {
    switch(cmd->tag) {
    case cmd_add_component:
      alias_ecs_add_component_to_entity(instance, cmd->entity, cmd->component, cmd + 1);
      break;
    case cmd_remove_component:
      alias_ecs_remove_component_from_entity(instance, cmd->entity, cmd->component);
      break;
    case cmd_despawn:
      alias_ecs_despawn(instance, 1, &cmd->entity);
      break;
    }
    cmd = (struct Cmd *)((uint8_t *)cmd + cmd->size);
  }
}
