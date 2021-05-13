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
#define WALL_SIZE      20

struct {
  Entity paddle;
  Entity hold_ball;
  Entity pin_constraint;
  int life;
} g;

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
static void _paused_begin(void * ud) {
  physics_set_speed(0.0f);
}

static void _paused_frame(void * ud) {
  extern struct State playing;

  if(IsKeyPressed('P')) {
    state_pop();
    return;
  }

  DrawText("GAME PAUSED", SCREEN_WIDTH / 2 - MeasureText("GAME PAUSED", 40) / 2, SCREEN_HEIGHT/2 - 40, 40, GRAY);
}

static void _paused_end(void * ud) {
  physics_set_speed(1.0f);
}

struct State paused = {
  .begin = _paused_begin,
  .frame = _paused_frame,
  .end = _paused_end
};

// =============================================================================================================================================================
enum CollisionType {
  ct_paddle,
  ct_wall,
  ct_goal,
  ct_brick,
  ct_ball
};

struct Collision2D_data paddle_collision_data = {
  .collision_type = ct_paddle,
  .kind = Collision2D_box,
  .width = SCREEN_WIDTH / 10.0f,
  .height = 20.0f,
  .radius = 1.0f,
  .elasticity = 0.99
};

struct Collision2D_data ball_collision_data = {
  .collision_type = ct_ball,
  .kind = Collision2D_circle,
  .radius = 7.0f,
  .elasticity = 0.99
};

struct Collision2D_data vertical_wall_collision_data = {
  .collision_type = ct_wall,
  .kind = Collision2D_box,
  .width = WALL_SIZE,
  .height = SCREEN_HEIGHT,
  .elasticity = 0.99
};

struct Collision2D_data horizontal_wall_collision_data = {
  .collision_type = ct_wall,
  .kind = Collision2D_box,
  .width = SCREEN_WIDTH,
  .height = WALL_SIZE,
  .elasticity = 0.99
};

struct Collision2D_data brick_collision_data = {
  .collision_type = ct_brick,
  .kind = Collision2D_box,
  .width = (SCREEN_WIDTH - WALL_SIZE * 2) / BRICKS_PER_LINE,
  .height = WALL_SIZE,
  .elasticity = 0.99
};

void _teleport_ball_to_paddle(Entity ball) {
  const struct Body2D * paddle_body = Body2D_read(g.paddle);
  struct Body2D * ball_body = Body2D_write(ball);

  cpBodySetPosition(ball_body->body, cpvadd(cpBodyGetPosition(paddle_body->body), cpv(0, -15)));
}

void _hold_ball(Entity ball) {
  g.hold_ball = ball;
  g.pin_constraint = SPAWN(( Constraint2D, .kind = Constraint2D_pin, .body_a = g.paddle, .body_b = g.hold_ball ));
}

void _release_ball(void) {
  SPAWN(( AddImpulse2D, .body = g.hold_ball, .impulse[1] = -5 ));
  Constraint2D_write(g.pin_constraint)->inactive = true;

  g.hold_ball = 0;
}

static void _playing_begin(void * ud) {
  g.paddle = SPAWN(
                ( Transform2D, .x = SCREEN_WIDTH / 2.0f, .y = SCREEN_HEIGHT * 7.0f / 8.0f )
              , ( DrawRectangle, .width = SCREEN_WIDTH / 10.0f, .height = 20.0f, .color = BLACK )
              , ( Body2D, .kind = Body2D_kinematic )
              , ( Collision2D, .data = &paddle_collision_data )
              , ( Velocity2D, .x = 0.0f, .y = 0.0f, .a = 0.0f )
              );

  Entity ball = SPAWN(
                   ( Transform2D, .x = SCREEN_WIDTH / 2.0f, .y = SCREEN_HEIGHT * 7.0f / 8.0f - 17.0f )
                 , ( DrawCircle, .radius = 7, .color = MAROON )
                 , ( Body2D, .kind = Body2D_dynamic, .mass = 1.0f, .moment = 1.0f )
                 , ( Collision2D, .data = &ball_collision_data )
                 );

  int game_space_l = WALL_SIZE;
  int game_space_r = SCREEN_WIDTH - WALL_SIZE;
  int game_space_t = WALL_SIZE;
  int game_space_b = SCREEN_HEIGHT;
  int game_space_width = game_space_r - game_space_l;
  int game_space_height = game_space_b - game_space_t;

  int brick_width = game_space_width / BRICKS_PER_LINE;
  int brick_height = 20;

  // top wall
  SPAWN(
      ( Transform2D, .x = SCREEN_WIDTH / 2.0f, .y = WALL_SIZE / 2.0f )
    , ( DrawRectangle, .width = SCREEN_WIDTH, .height = WALL_SIZE, .color = DARKGRAY )
    , ( Body2D, .kind = Body2D_static )
    , ( Collision2D, .data = &horizontal_wall_collision_data )
    );

  // left wall
  SPAWN(
      ( Transform2D, .x = WALL_SIZE / 2.0f, .y = SCREEN_HEIGHT / 2.0f )
    , ( DrawRectangle, .width = WALL_SIZE, .height = SCREEN_HEIGHT, .color = DARKGRAY )
    , ( Body2D, .kind = Body2D_static )
    , ( Collision2D, .data = &vertical_wall_collision_data )
    );

  // right wall
  SPAWN(
      ( Transform2D, .x = SCREEN_WIDTH - (WALL_SIZE / 2.0f), .y = SCREEN_HEIGHT / 2.0f )
    , ( DrawRectangle, .width = WALL_SIZE, .height = SCREEN_HEIGHT, .color = DARKGRAY )
    , ( Body2D, .kind = Body2D_static )
    , ( Collision2D, .data = &vertical_wall_collision_data )
    );

  for(uint32_t i = 0; i < LINES_OF_BRICKS; i++) {
    for(uint32_t j = 0; j < BRICKS_PER_LINE; j++) {
      SPAWN(
          ( Transform2D, .x = j * brick_width + brick_width / 2.0f + game_space_l, .y = (i + 1) * brick_height + game_space_t )
        , ( DrawRectangle, .width = brick_width, .height = brick_height, .color = (i + j) % 2 ? GRAY : DARKGRAY )
        , ( Body2D, .kind = Body2D_static )
        , ( Collision2D, .data = &brick_collision_data )
        );
    }
  }

  // -

  cpSpaceSetGravity(physics_space(), cpv(0.0f, 0.0f));
  cpSpaceSetDamping(physics_space(), 1.0f);

  _hold_ball(ball);
}

static void _playing_frame(void * ud) {
  extern struct State paused;

  if(IsKeyPressed('P')) {
    state_push(&paused);
    return;
  }

  if(g.hold_ball && IsKeyPressed(KEY_SPACE)) {
    _release_ball();
  }

  Velocity2D_write(g.paddle)->x = (IsKeyDown(KEY_RIGHT) ? 500 : 0) - (IsKeyDown(KEY_LEFT) ? 500 : 0);
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
    physics_frame();
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
