#include <alias/ecs.h>
#include <raylib.h>

#define DEFINE_FONT(IDENT, PATH) \
  Font * IDENT () {              \
    static Font font, * ptr = NULL; \
    if(ptr == NULL) {                  \
      font = LoadFont(PATH);     \
      ptr = &font; \
    }                            \
    return ptr;                 \
  }

DEFINE_FONT(Romulus, "resources/fonts/romulus.png");

int main(int argc, char * argv []) {
  int screen_width = 800;
  int screen_height = 600;

  InitWindow(screen_width, screen_height, "a_cave");

  SetTargetFPS(60);

  while(!WindowShouldClose()) {
    BeginDrawing();

    ClearBackground(BLACK);

    DrawTextEx(*Romulus(), "A CAVE", (Vector2) { 0, 0 }, Romulus()->baseSize * 2.0f, 3, DARKPURPLE);

    EndDrawing();
  }
  
  return 0;
}
