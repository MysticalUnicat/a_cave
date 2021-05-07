#include "util.h"

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

static inline void simulate(void) {
  //ECS(begin_recording, World(), cbuf);
  float time = GetTime();

  QUERY(World(), ( write, Position, pos ), ( read, AnimatePosition, ani )) {
    float t = (time - ani->start) / (ani->end - ani->end);
    if(time > ani->end) {
      pos->pos = ani->to;
      //ECS(cmd_remove_component, cbuf, entity, AnimatePosition_component());
      return;
    }
    
    pos->pos.x = ani->from.x + (ani->to.x - ani->from.x) * t;
    pos->pos.y = ani->from.y + (ani->to.y - ani->from.y) * t;
  }

  //ECS(end_recording, World(), cbuf);
  //ECS(execute_commands, World(), 1, &cbuf);
}

static inline void draw(void) {
  BeginDrawing();

  ClearBackground(BLACK);

  QUERY(World(), ( read, Position, position ), ( read, Text, text )) {
    DrawTextEx(*text->font, text->text, position->pos, text->font->baseSize * text->size, 3, text->color);
  }

  EndDrawing();
}

int main(int argc, char * argv []) {
  int screen_width = 800;
  int screen_height = 600;

  InitWindow(screen_width, screen_height, "a_cave");

  SetTargetFPS(60);

  SPAWN(
    World(),
    ( Position, .pos = (Vector2) { .x = 0.0f, .y = 0.0f }  ),
    (     Text, .text = "A CAVE", .font = Romulus(), .size = 2.0f, .color = DARKPURPLE )
  );

  //alias_ecs_CommandBuffer cbuf;
  //ECS(create_command_buffer, World(), &cbuf);

  while(!WindowShouldClose()) {
    simulate();
    draw();
  }

  CloseWindow();
  
  return 0;
}
