#include "util.h"

#include "texture.h"

#include <stdlib.h>
#include <string.h>

uint32_t FRAME = 0;

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

void animate_position_to(struct CmdBuf * cbuf, Entity entity, Vector2 to, float seconds) {
  const struct Position * pos;
  ECS(read_entity_component, World(), entity, Position_component(), (const void **)&pos);
  
  struct AnimatePosition ani, *a;
  ani.from = pos->pos;
  ani.to = to;
  ani.start = GetTime();
  ani.end = ani.start + seconds;

  ECS(add_component_to_entity, World(), entity, AnimatePosition_component(), &ani);
}

DEFINE_COMPONENT(World(), Sprite, {
  const char * path;
  TextureHandle texture;
  Vector2 offset;
  float size;
  Color tint;
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

static inline void simulate(alias_ecs_Instance * world) {
  static struct CmdBuf cbuf = { 0, 0, 0 };
  
  float time = GetTime();

  CmdBuf_begin_recording(&cbuf);

  QUERY(world, ( write, Position, pos ), ( read, AnimatePosition, ani )) {
    float t = (time - ani->start) / (ani->end - ani->start);
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

static inline void draw(alias_ecs_Instance * world) {
  BeginDrawing();

  ClearBackground(BLACK);

  QUERY(world
    , ( read, Position, position )
    , ( read, Sprite, sprite )
  ) {
    if(sprite->texture == 0) {
      sprite->texture = get_texture_handle(sprite->path, false);
    }
    DrawTextureEx(get_texture(sprite->texture), position->pos, 0.0f, sprite->size, sprite->tint);
  }

  QUERY(world, ( read, Position, position ), ( read, Text, text )) {
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

int main(int argc, char * argv []) {
  int screen_width = 800;
  int screen_height = 600;

  InitWindow(screen_width, screen_height, "a_cave");

  SetTargetFPS(60);

  Entity text = SPAWN(
    World(),
    ( Position, .pos = (Vector2) { .x = 50.0f, .y = 10.0f } ),
    (     Text, .text = "A CAVE", .font = Romulus(), .size = 2.0f, .color = DARKPURPLE )
  );

  animate_position_to(NULL, text, (Vector2) { 700.0f, 500.0f }, 5.0f);

  Entity player = SPAWN(
    World(),
    ( Position, .pos = (Vector2) { .x = 0.0f, .y = 0.0f } ),
    (   Sprite, .path = "assets/characters/3SAMURAI-CHIBI/PNG-FILE/Samurai01/01-Idle/2D_SM01_Idle_000.png", .size = 32.0f, .tint = WHITE )
  );

  while(!WindowShouldClose()) {
    cleanup_textures();
    
    simulate(World());
    draw(World());
  }

  CloseWindow();
  
  return 0;
}
