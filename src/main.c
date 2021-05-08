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

#define DEBUGf(X) printf(#X " = %g\n", X)

static inline void simulate(void) {
  static struct CmdBuf cbuf = { 0, 0, 0 };
  
  float time = GetTime();

  CmdBuf_begin_recording(&cbuf);

  QUERY(World(), ( write, Position, pos ), ( read, AnimatePosition, ani )) {
    DEBUGf(ani->from.x);
    DEBUGf(ani->from.y);
    DEBUGf(ani->to.x);
    DEBUGf(ani->to.y);
    DEBUGf(ani->start);
    DEBUGf(ani->end);
    
    float t = (time - ani->start) / (ani->end - ani->end);
    if(time > ani->end) {
      pos->pos = ani->to;
      CmdBuf_remove_component(&cbuf, entity, AnimatePosition_component());

      printf("remove animation at %g > %g\n", time, ani->end);
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
  
  struct AnimatePosition ani, *a;
  ani.from = pos->pos;
  ani.to = to;
  ani.start = GetTime();
  ani.end = ani.start + seconds;

  ECS(add_component_to_entity, World(), entity, AnimatePosition_component(), &ani);

  ECS(read_entity_component, World(), entity, AnimatePosition_component(), (const void **)&a);

  assert(fabs(ani.from.x - a->from.x) < 0.001f);
  assert(fabs(ani.from.y - a->from.y) < 0.001f);
  assert(fabs(ani.to.x - a->to.x) < 0.001f);
  assert(fabs(ani.to.y - a->to.y) < 0.001f);
  assert(fabs(ani.start - a->start) < 0.001f);
  assert(fabs(ani.end - a->end) < 0.001f);
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

  animate_position_to(NULL, text, (Vector2) { 100.0f, 20.0f }, 5.0f);

  while(!WindowShouldClose()) {
    simulate();
    draw();
  }

  CloseWindow();
  
  return 0;
}
