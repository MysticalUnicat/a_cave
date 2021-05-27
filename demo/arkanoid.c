#include "../src/util.h"

#include "../src/transform.h"
#include "../src/physics.h"
#include "../src/render.h"
#include "../src/event.h"
#include "../src/state.h"

alias_ecs_Instance * g_world;

#define MAX_PLAYER_LIFE  5
#define LINES_OF_BRICKS  5
#define BRICKS_PER_LINE 20

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define WALL_SIZE      20

enum CollisionType {
  ct_ball,
  ct_paddle,
  ct_wall,
  ct_goal,
  ct_brick,
};

struct {
  Entity paddle;
  Entity ball;
  Entity hold_ball;
  Entity pin_constraint;
  Entity test_rotate_1;
  Entity test_rotate_2;
  Entity test_rotate_3;
  int life;
} g;

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
void _teleport_ball_to_paddle(Entity ball) {
  const struct Body2D * paddle_body = Body2D_read(g.paddle);
  struct Body2D * ball_body = Body2D_write(ball);

  cpBodySetPosition(ball_body->body, cpvadd(cpBodyGetPosition(paddle_body->body), cpv(0, -15)));
}

void _hold_ball(Entity ball) {
  g.hold_ball = ball;
  Constraint2D_write(g.pin_constraint)->inactive = false;
}

void _release_ball(void) {
  SPAWN_EVENT(( AddImpulse2D, .body = g.hold_ball, .impulse.y = -500 ));
  Constraint2D_write(g.pin_constraint)->inactive = true;

  g.hold_ball = 0;
}

static void _playing_begin(void * ud) {
  _hold_ball(g.ball);
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

  alias_Rotation2D_write(g.test_rotate_1)->value = GetTime() / 3.0f;
  alias_Rotation2D_write(g.test_rotate_2)->value = GetTime() / 5.0f;
  alias_Rotation2D_write(g.test_rotate_3)->value = GetTime() / 9.0f;

  Velocity2D_write(g.paddle)->x = (IsKeyDown(KEY_RIGHT) ? 500 : 0) - (IsKeyDown(KEY_LEFT) ? 500 : 0);

  QUERY_EVENT(( read, Contact2D, c )) {
    if(c->kind == Contact2D_begin && c->type_a == ct_ball && c->type_b == ct_brick) {
      Collision2D_write(c->shape_b)->inactive = true;
      alias_ecs_remove_component_from_entity(g_world, c->shape_b, DrawRectangle_component());
    }

    if(c->type_a == ct_ball && c->type_b == ct_goal) {
      _teleport_ball_to_paddle(c->body_a);
      _hold_ball(c->body_a);
    }
  }
}

struct State playing = {
  .begin = _playing_begin,
  .frame = _playing_frame
};

// =============================================================================================================================================================
static void _start_begin(void * ud) {
  g.paddle = SPAWN(
                ( alias_Translation2D, .value.x = SCREEN_WIDTH / 2.0f, .value.y = SCREEN_HEIGHT * 7.0f / 8.0f )
              , ( DrawRectangle, .width = SCREEN_WIDTH / 10.0f, .height = 20.0f, .color = BLACK )
              , ( Body2D, .kind = Body2D_kinematic )
              , ( Collision2D, .collision_type = ct_paddle
                             , .kind = Collision2D_box
                             , .width = SCREEN_WIDTH / 10.0f
                             , .height = 20.0f
                             , .radius = 1.0f
                             , .elasticity = 0.99
                             )
              , ( Velocity2D, .x = 0.0f, .y = 0.0f, .a = 0.0f )
              );

  alias_R arm_length = 40.0f;

  g.test_rotate_1 = SPAWN(
      ( alias_Translation2D, .value.x = SCREEN_WIDTH / 10.0f )
    , ( alias_Rotation2D, .value = 0.0f )
    , ( alias_Parent2D, .value = g.paddle )
    , ( DrawCircle, .radius = 7, .color = YELLOW )
    );
  SPAWN(
      ( alias_Translation2D, .value.x = arm_length )
    , ( alias_Parent2D, .value = g.test_rotate_1 )
    , ( DrawRectangle, .width = arm_length, .height = 2.0f, .color = BLACK )
    );

  g.test_rotate_2 = SPAWN(
      ( alias_Translation2D, .value.x = arm_length )
    , ( alias_Rotation2D, .value = 0.0f )
    , ( alias_Parent2D, .value = g.test_rotate_1 )
    , ( DrawCircle, .radius = 7, .color = YELLOW )
    );
  SPAWN(
      ( alias_Translation2D, .value.x = arm_length )
    , ( alias_Parent2D, .value = g.test_rotate_2 )
    , ( DrawRectangle, .width = arm_length, .height = 2.0f, .color = BLACK )
    );

  g.test_rotate_3 = SPAWN(
      ( alias_Translation2D, .value.x = arm_length )
    , ( alias_Rotation2D, .value = 0.0f )
    , ( alias_Parent2D, .value = g.test_rotate_2 )
    , ( DrawCircle, .radius = 7, .color = YELLOW )
    );
  SPAWN(
      ( alias_Translation2D, .value.x = arm_length )
    , ( alias_Parent2D, .value = g.test_rotate_3 )
    , ( DrawRectangle, .width = arm_length, .height = 2.0f, .color = BLACK )
    );

  int game_space_l = WALL_SIZE;
  int game_space_r = SCREEN_WIDTH - WALL_SIZE;
  int game_space_t = WALL_SIZE;
  int game_space_b = SCREEN_HEIGHT;
  int game_space_width = game_space_r - game_space_l;
  int game_space_height = game_space_b - game_space_t;

  int brick_width = game_space_width / BRICKS_PER_LINE;
  int brick_height = 20;

  Entity level = SPAWN(( Body2D, .kind = Body2D_static ));

  #define BOX(T, W, H) .collision_type = T, .kind = Collision2D_box, .width = W, .height = H, .elasticity = 0.99f, .radius = 1.0f
  #define WALL(W, H) BOX(ct_wall, W, H)

  // top wall
  SPAWN(
      ( alias_Translation2D, .value.x = SCREEN_WIDTH / 2.0f, .value.y = WALL_SIZE / 2.0f )
    , ( DrawRectangle, .width = SCREEN_WIDTH, .height = WALL_SIZE, .color = DARKGRAY )
    , ( Collision2D, .body = level, WALL(SCREEN_WIDTH, WALL_SIZE) )
    );

  // left wall
  SPAWN(
      ( alias_Translation2D, .value.x = WALL_SIZE / 2.0f, .value.y = SCREEN_HEIGHT / 2.0f )
    , ( DrawRectangle, .width = WALL_SIZE, .height = SCREEN_HEIGHT, .color = DARKGRAY )
    , ( Collision2D, .body = level, WALL(WALL_SIZE, SCREEN_HEIGHT) )
    );

  // right wall
  SPAWN(
      ( alias_Translation2D, .value.x = SCREEN_WIDTH - (WALL_SIZE / 2.0f), .value.y = SCREEN_HEIGHT / 2.0f )
    , ( DrawRectangle, .width = WALL_SIZE, .height = SCREEN_HEIGHT, .color = DARKGRAY )
    , ( Collision2D, .body = level, WALL(WALL_SIZE, SCREEN_HEIGHT) )
    );

  SPAWN(
      ( alias_Translation2D, .value.x = SCREEN_WIDTH / 2.0f, .value.y = SCREEN_HEIGHT - WALL_SIZE )
    , ( Collision2D, .body = level, BOX(ct_goal, SCREEN_WIDTH, WALL_SIZE) )
    );

  for(uint32_t i = 0; i < LINES_OF_BRICKS; i++) {
    for(uint32_t j = 0; j < BRICKS_PER_LINE; j++) {
      SPAWN(
          ( alias_Translation2D, .value.x = j * brick_width + brick_width / 2.0f + game_space_l, .value.y = (i + 1) * brick_height + game_space_t )
        , ( DrawRectangle, .width = brick_width, .height = brick_height, .color = (i + j) % 2 ? GRAY : DARKGRAY )
        , ( Collision2D, .body = level, BOX(ct_brick, (SCREEN_WIDTH - WALL_SIZE * 2) / BRICKS_PER_LINE, WALL_SIZE) )
        );
    }
  }

  // -

  physics_set_gravity(0.0f, 0.0f);
  physics_set_damping(1.0f);

  g.ball = SPAWN(
                   ( alias_Translation2D, .value.x = SCREEN_WIDTH / 2.0f, .value.y = SCREEN_HEIGHT * 7.0f / 8.0f - 17.0f )
                 , ( DrawCircle, .radius = 7, .color = MAROON )
                 , ( Body2D, .kind = Body2D_dynamic, .mass = 1.0f, .moment = 1.0f )
                 , ( Collision2D, .collision_type = ct_ball
                                , .kind = Collision2D_circle
                                , .radius = 7.0f
                                , .elasticity = 0.99
                                )
                 );
  
  g.pin_constraint = SPAWN(( Constraint2D, .kind = Constraint2D_pin, .body_a = g.paddle, .body_b = g.ball ));

  state_pop();
  state_push(&playing);
}

struct State start = {
  .begin = _start_begin
};

// =============================================================================================================================================================
int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "sample game: arkanoid");

  ECS(create_instance, NULL, &g_world);
  transform_init();

  state_push(&start);

  SetTargetFPS(60);

  while(!WindowShouldClose()) {
    event_update();
    transform_update();
    physics_update();
    render_frame();

    state_frame();
  }
}

