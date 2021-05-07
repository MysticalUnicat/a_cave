#include "util.h"

DEFINE_FONT(Romulus, "resources/fonts/romulus.png")

DEFINE_WORLD(World)

DEFINE_COMPONENT(World(), Position, {
  float x;
  float y;
})

DEFINE_COMPONENT(World(), Text, {
  const char * text;
});

int main(int argc, char * argv []) {
  int screen_width = 800;
  int screen_height = 600;

  InitWindow(screen_width, screen_height, "a_cave");

  SetTargetFPS(60);

  SPAWN(
    World(),
    ( Position, .x = 0.0f, .y = 0.0f ),
    (     Text, .text = "A CAVE"     )
  );

  while(!WindowShouldClose()) {
    BeginDrawing();

    ClearBackground(BLACK);

    QUERY(World(), ( read, Position, position ), ( read, Text, text )) {
      DrawTextEx(*Romulus(), text->text, (Vector2) { .x = position->x, .y = position->y }, Romulus()->baseSize * 2.0f, 3, DARKPURPLE);
    }

    QUERY(World(), ( write, Position, position )) {
      position->x += GetFrameTime() * 10;
    }

    EndDrawing();
  }

  CloseWindow();
  
  return 0;
}
