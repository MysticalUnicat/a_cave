#define ARKANOID 1

#include "util.h"

#include "transform.h"
#include "physics.h"
#include "render.h"

alias_ecs_Instance * g_world;

#if ARKANOID

#define MAX_PLAYER_LIFE  5
#define LINES_OF_BRICKS  5
#define BRICKS_PER_LINE 20

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

Entity _paddle;
Entity _held_ball;
int _life;

struct State {
  struct State * prev;
  void * ud;
  void (*begin)(void * ud);
  void (*frame)(void * ud);
  void (*background)(void * ud);
  void (*pause)(void * ud);
  void (*unpause)(void * ud);
  void (*end)(void * ud);
} * state_current = NULL;

void state_push(struct State * state) {
  if(state_current != NULL && state_current->pause != NULL) {
    state_current->pause(state_current->ud);
  }
  state->prev = state_current;
  state_current = state;
  if(state->begin != NULL) {
    state->begin(state->ud);
  }
}

void state_pop(void) {
  if(state_current == NULL) {
    return;
  }
  if(state_current->end != NULL) {
    state_current->end(state_current->ud);
  }
  state_current = state_current->prev;
  if(state_current->unpause != NULL) {
    state_current->unpause(state_current->ud);
  }
}

void state_frame(void) {
  struct State * current = state_current;
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

// =============================================================================================================================================================
static void _paused_frame(void * ud) {
  extern struct State playing;

  if(IsKeyPressed('P')) {
    state_pop();
    return;
  }
}

struct State paused = {
  .frame = _paused_frame
};

// =============================================================================================================================================================
static void _playing_begin(void * ud) {
  _paddle = SPAWN(
                ( Transform2D, .x = SCREEN_WIDTH / 2.0f, .y = SCREEN_HEIGHT * 7.0f / 8.0f )
              , ( DrawRectangle, .width = SCREEN_HEIGHT / 10.0f, .height = 20.0f, .color = BLACK )
              );

  _held_ball = SPAWN(
                   ( Transform2D, .x = SCREEN_WIDTH / 2.0f, .y = SCREEN_HEIGHT * 7.0f / 8.0f - 30.0f )
                 , ( DrawCircle, .radius = 7, .color = MAROON )
                 );

  float brick_width = (float)SCREEN_WIDTH / (BRICKS_PER_LINE + 1);
  float brick_height = 20;
  float initial_down_position = 50;

  for(uint32_t i = 0; i < LINES_OF_BRICKS; i++) {
    for(uint32_t j = 0; j < BRICKS_PER_LINE; j++) {
      SPAWN(
          ( Transform2D, .x = j * brick_width + brick_width / 2.0f, .y = i * brick_height + initial_down_position )
        , ( DrawRectangle, .width = brick_width, .height = brick_height, .color = (i + j) % 2 ? GRAY : DARKGRAY )
        );
    }
  }
}

static void _playing_frame(void * ud) {
  extern struct State paused;
  //float delta = (IsKeyDown(KEY_RIGHT) ? 5 : 0) - (IsKeyDown(KEY_LEFT) ? 5 : 0);

  if(IsKeyPressed('P')) {
    state_push(&paused);
  }

  //physics_frame();
}

struct State playing = {
  .begin = _playing_begin,
  .frame = _playing_frame
};

// =============================================================================================================================================================
int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "sample game: arkanoid");

  ECS(create_instance, NULL, &g_world);

  state_push(&playing);

  SetTargetFPS(60);

  while(!WindowShouldClose()) {
    state_frame();
    render_frame();
  }
}
#else
#include "game.h"
#include "texture.h"
#include "stats.h"

#include <stdlib.h>
#include <string.h>

//DEFINE_FONT(Romulus, "resources/fonts/romulus.png")

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
  struct AnimatePosition ani, *a;
  ani.from = Position_read(entity)->pos;
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
  BeginMode2D(game_state.camera);

  ClearBackground(BLACK);

  QUERY(world
    , ( read, Position, position )
    , ( write, Sprite, sprite )
  ) {
    if(sprite->texture == 0) {
      sprite->texture = get_texture_handle(sprite->path, false);
    }
    
    DrawTextureEx(get_texture(sprite->texture), position->pos, 0.0f, sprite->size, sprite->tint);

    STAT(texture draw);
  }

  EndMode2D();

  QUERY(world, ( read, Position, position ), ( read, Text, text )) {
    DrawTextEx(
        *text->font
      , text->text
      , position->pos
      , text->font->baseSize * text->size
      , 3
      , text->color
    );

    STAT(text draw);
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
    ( Position, .pos.x = 50.0f, .pos.y = 10.0f ),
    (     Text, .text = "A CAVE", .font = Romulus(), .size = 2.0f, .color = DARKPURPLE )
  );

  animate_position_to(NULL, text, (Vector2) { 700.0f, 500.0f }, 5.0f);

  memset(&game_state, 0, sizeof(game_state));
  game_state.camera.zoom = 1.0f;

  game_state.player = SPAWN(
    World(),
    ( Position, .pos.x = 0.0f, .pos.y = 0.0f ),
    (   Sprite, .path = "assets/characters/3SAMURAI-CHIBI/PNG-FILE/Samurai01/01-Idle/2D_SM01_Idle_000.png", .size = 32.0f, .tint = WHITE )
  );

  while(!WindowShouldClose()) {
    // the order does not matter? *shrug*
    cleanup_textures();

    simulate(World());

    draw(World());

    stat_frame();
    //PRINT_STAT_PER_FRAME(texture draw);
    //PRINT_STAT_PER_FRAME(text draw);
  }

  CloseWindow();
  
  return 0;
}
#endif
