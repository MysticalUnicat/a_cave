#include "util.h"

DEFINE_FONT(Romulus, "resources/fonts/romulus.png")

DEFINE_WORLD(World)

DEFINE_COMPONENT(World(), Position, {
  Vector2 position;
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
    ( Position, .position = (Vector2) { 0, 0 } ),
    (     Text, .text = "A CAVE" )
  );

  while(!WindowShouldClose()) {
    BeginDrawing();

    ClearBackground(BLACK);

    QUERY(World(), ( read, Position, position ), ( read, Text, text )) {
      DrawTextEx(*Romulus(), text->text, position->position, Romulus()->baseSize * 2.0f, 3, DARKPURPLE);
    }

    QUERY(World(), ( write, Position, position )) {
      position->position.x += 1;
    }

    EndDrawing();
  }

  CloseWindow();
  
  return 0;
}
