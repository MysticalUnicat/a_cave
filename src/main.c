#include <alias/ecs.h>
#include <raylib.h>

#define LAZY_GLOBAL(TYPE, IDENT, CODE) \
  TYPE IDENT (void) {                  \
    static TYPE inner;                 \
    static int init = 0;               \
    if(init == 0) {                    \
      CODE                             \
      init = 1;                        \
    }                                  \
    return inner;                      \
  }

#define LAZY_GLOBAL_PTR(TYPE, IDENT, CODE) \
  TYPE * IDENT (void) {                    \
    static TYPE inner;                     \
    static TYPE * __ptr = NULL;            \
    if(__ptr == NULL) {                    \
      CODE                                 \
      __ptr = &inner;                      \
    }                                      \
    return __ptr;                          \
  }

#define DEFINE_FONT(IDENT, PATH) LAZY_GLOBAL_PTR(Font, IDENT, inner = LoadFont(PATH);)

#define DEFINE_WORLD(IDENT) LAZY_GLOBAL(alias_ecs_Instance *, IDENT, alias_ecs_create_instance(NULL, &inner);)

#define DEFINE_COMPONENT(WORLD, IDENT, FIELDS) \
  struct IDENT FIELDS; \
  LAZY_GLOBAL(alias_ecs_ComponentHandle, IDENT##_component, alias_ecs_register_component(WORLD, &(alias_ecs_ComponentCreateInfo) { .size = sizeof(struct IDENT) }, &inner);)

#define DEFINE_QUERY(WORLD, R_COUNT, ...) \
  LAZY_GLOBAL(

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

  while(!WindowShouldClose()) {
    BeginDrawing();

    ClearBackground(BLACK);

    DrawTextEx(*Romulus(), "A CAVE", (Vector2) { 0, 0 }, Romulus()->baseSize * 2.0f, 3, DARKPURPLE);

    EndDrawing();
  }

  CloseWindow();
  
  return 0;
}
