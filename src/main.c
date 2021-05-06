#include <alias/ecs.h>
#include <raylib.h>

#define LAZY_GLOBAL(TYPE, IDENT, ...) \
  TYPE IDENT (void) {                 \
    static TYPE inner;                \
    static int init = 0;              \
    if(init == 0) {                   \
      __VA_ARGS__                     \
      init = 1;                       \
    }                                 \
    return inner;                     \
  }

#define LAZY_GLOBAL_PTR(TYPE, IDENT, ...) \
  TYPE * IDENT (void) {                   \
    static TYPE inner;                    \
    static TYPE * __ptr = NULL;           \
    if(__ptr == NULL) {                   \
      __VA_ARGS__                         \
      __ptr = &inner;                     \
    }                                     \
    return __ptr;                         \
  }

#define DEFINE_FONT(IDENT, PATH) LAZY_GLOBAL_PTR(Font, IDENT, inner = LoadFont(PATH);)

#define DEFINE_WORLD(IDENT) LAZY_GLOBAL(alias_ecs_Instance *, IDENT, alias_ecs_create_instance(NULL, &inner);)

#define DEFINE_COMPONENT(WORLD, IDENT, FIELDS) \
  struct IDENT FIELDS; \
  LAZY_GLOBAL(alias_ecs_ComponentHandle, IDENT##_component, alias_ecs_register_component(WORLD, &(alias_ecs_ComponentCreateInfo) { .size = sizeof(struct IDENT) }, &inner);)

#define DEFINE_QUERY(WORLD, NAME, W_COUNT, ...)                   \
  LAZY_GLOBAL(alias_ecs_Query *, NAME,                            \
    alias_ecs_ComponentHandle handles[] = { __VA_ARGS__ };        \
    uint32_t handle_count = sizeof(handles) / sizeof(handles[0]); \
    alias_ecs_create_query(WORLD, &(alias_ecs_QueryCreateInfo) {  \
      .num_write_components = W_COUNT,                            \
      .write_components = handles,                                \
      .num_read_components = handle_count - W_COUNT,              \
      .read_components = handles + W_COUNT,                       \
    }, &inner);                                                   \
  )

#define _CAT1(x, y) x ## y
#define _CAT0(x, y) _CAT1(x, y)
#define CAT(x, y) _CAT0(x, y)

#define RUN_QUERY(WORLD, QUERY) \
  auto void CAT(query_fn_, __LINE__)(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data); \
  alias_ecs_execute_query(WORLD, QUERY, (alias_ecs_QueryCB) { CAT(query_fn_, __LINE__), NULL }); \
  auto void CAT(query_fn_, __LINE__)(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data)

DEFINE_FONT(Romulus, "resources/fonts/romulus.png")

DEFINE_WORLD(World)

DEFINE_COMPONENT(World(), Position, {
  Vector2 position;
})

DEFINE_COMPONENT(World(), Text, {
  const char * text;
});

DEFINE_QUERY(World(), RenderableText, 0, Position_component(), Text_component());

int main(int argc, char * argv []) {
  int screen_width = 800;
  int screen_height = 600;

  InitWindow(screen_width, screen_height, "a_cave");

  SetTargetFPS(60);

  while(!WindowShouldClose()) {
    BeginDrawing();

    ClearBackground(BLACK);

    //DrawTextEx(*Romulus(), "A CAVE", (Vector2) { 0, 0 }, Romulus()->baseSize * 2.0f, 3, DARKPURPLE);
    RUN_QUERY(World(), RenderableText()) {
      const struct Position * position = (const struct Position *)data[0];
      const struct Text * text = (const struct Text *)data[1];
      DrawTextEx(*Romulus(), text->text, position->position, Romulus()->baseSize * 2.0f, 3, DARKPURPLE);
    }

    EndDrawing();
  }

  CloseWindow();
  
  return 0;
}
