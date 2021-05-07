#include "util.h"

#include <stdlib.h>
#include <string.h>

DEFINE_FONT(Romulus, "resources/fonts/romulus.png")

DEFINE_WORLD(World)

DEFINE_COMPONENT(World(), Position, {
  Vector2 pos;
})

DEFINE_COMPONENT(World(), AnimatePosition, {
  Vector2 from;
  Vector2 to;
  float start;
  float end;
})

DEFINE_COMPONENT(World(), Text, {
  const char * text;
  Font * font;
  float size;
  Color color;
});

#define GRID_SIZE 32

static inline float to_grid(float x) {
  return floor(x / (float)GRID_SIZE) * (float)GRID_SIZE;
}

struct Cmd {
  uint32_t size;
  enum {
    cmd_add_component,
    cmd_remove_component,
    cmd_despawn
  } tag;
  alias_ecs_EntityHandle entity;
  alias_ecs_ComponentHandle component;
};

struct CmdBuf {
  void * start;
  void * pointer;
  void * end;
};

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
      //alias_ecs_despawn(instance, 1, &cmd->entity);
      break;
    }
    cmd = (struct Cmd *)((uint8_t *)cmd + cmd->size);
  }
}

static inline void simulate(void) {
  static struct CmdBuf cbuf = { 0, 0, 0 };
  
  float time = GetTime();

  CmdBuf_begin_recording(&cbuf);

  QUERY(World(), ( write, Position, pos ), ( read, AnimatePosition, ani )) {
    float t = (time - ani->start) / (ani->end - ani->end);
    if(time > ani->end) {
      pos->pos = ani->to;
      CmdBuf_remove_component(&cbuf, entity, AnimatePosition_component());
      return;
    }
    
    pos->pos.x = ani->from.x + (ani->to.x - ani->from.x) * t;
    pos->pos.y = ani->from.y + (ani->to.y - ani->from.y) * t;
  }

  CmdBuf_end_recording(&cbuf);
  CmdBuf_execute(&cbuf, World());
}

static inline void draw(void) {
  BeginDrawing();

  ClearBackground(BLACK);

  QUERY(World(), ( read, Position, position ), ( read, Text, text )) {
    printf("draw %p %p\n", position, text);
    DrawTextEx(
        *text->font
      , text->text
      , position->pos
      , text->font->baseSize * text->size
      , 3
      , text->color
    );
  }

  EndDrawing();
}

void animate_position_to(struct CmdBuf * cbuf, Entity entity, Vector2 to, float seconds) {
  const struct Position * pos;
  ECS(read_entity_component, World(), entity, Position_component(), (const void **)&pos);
  struct AnimatePosition ani;
  ani.from = pos->pos;
  ani.to = to;
  ani.start = GetTime();
  ani.end = ani.start + seconds;
  ECS(add_component_to_entity, World(), entity, AnimatePosition_component(), &ani);
}

int main(int argc, char * argv []) {
  int screen_width = 800;
  int screen_height = 600;

  InitWindow(screen_width, screen_height, "a_cave");

  SetTargetFPS(60);

  Entity text = SPAWN(
    World(),
    ( Position, .pos = (Vector2) { .x = 0.0f, .y = 0.0f }  ),
    (     Text, .text = "A CAVE", .font = Romulus(), .size = 2.0f, .color = DARKPURPLE )
  );

  //animate_position_to(NULL, text, (Vector2) { 100.0f, 20.0f }, 5.0f);

  while(!WindowShouldClose()) {
    simulate();
    draw();
  }

  CloseWindow();
  
  return 0;
}
