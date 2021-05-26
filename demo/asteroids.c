#include "../src/util.h"

#include "../src/transform.h"
#include "../src/physics.h"
#include "../src/render.h"
#include "../src/event.h"
#include "../src/state.h"

alias_ecs_Instance * g_world;

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// =============================================================================================================================================================

static void _start_begin(void * ud) {

}

struct State start = {
  .begin = _start_begin,
};

// =============================================================================================================================================================

int main(int argc, char * * argv) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "sample game: arkanoid");

  ECS(create_instance, NULL, &g_world);

  state_push(&start);

  SetTargetFPS(60);

  while(!WindowShouldClose()) {
    event_update();
    state_frame();
    physics_update();
    render_frame();
  }
}

