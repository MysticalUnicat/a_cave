#include "engine.h"

#include <alias/ui.h>
#include <alias/data_structure/inline_list.h>

#define UI_NUM_VERTEXES (1024 * 1024)
#define UI_NUM_INDEXES  (1024 * 1024)
#define UI_NUM_GROUPS   1024

#define USE_UV 0

// why does this not put things in a namespace?!
#define Image raylib_Image
#define Color raylib_Color
#include <raylib.h>
#include <rlgl.h>
#undef Image
#undef Color

const struct alias_Color alias_Color_RAYWHITE = (struct alias_Color) { 0.96, 0.96, 0.96, 0.96 };

static inline raylib_Color alias_Color_to_raylib_Color(struct alias_Color c) {
  return (raylib_Color) { c.r * 255.0, c.g * 255.0, c.b * 255.0, c.a * 255.0 };
}

#if USE_UV
#include <uv.h>

static uv_loop_t _loop;
static uv_run_mode _loop_mode;
static uv_timer_t _frame_timer;
static bool _running;

static inline void _frame_timer_f(uv_timer_t * t);
#endif

static alias_ecs_Instance * _ecs;

static alias_ui * _ui;
static uint32_t _ui_indexes_data[UI_NUM_INDEXES];
static struct _ui_Vertex {
  float xy[2];
  float rgba[4];
  float st[2];
} _ui_vertexes_data[UI_NUM_VERTEXES];
static alias_memory_SubBuffer _ui_indexes = {
    .pointer = _ui_indexes_data
  , .count = UI_NUM_INDEXES
  , .stride = sizeof(_ui_indexes_data[0])
  , .type_format = alias_memory_Format_Uint32
  , .type_length = 1
};
static alias_memory_SubBuffer _ui_vertexes_xy = {
    .pointer = (uint8_t *)_ui_vertexes_data + offsetof(struct _ui_Vertex, xy)
  , .count = UI_NUM_VERTEXES
  , .stride = sizeof(_ui_vertexes_data[0])
  , .type_format = alias_memory_Format_Float32
  , .type_length = 2
};
static alias_memory_SubBuffer _ui_vertexes_rgba = {
    .pointer = (uint8_t *)_ui_vertexes_data + offsetof(struct _ui_Vertex, rgba)
  , .count = UI_NUM_VERTEXES
  , .stride = sizeof(_ui_vertexes_data[0])
  , .type_format = alias_memory_Format_Float32
  , .type_length = 4
};
static alias_memory_SubBuffer _ui_vertexes_st = {
    .pointer = (uint8_t *)_ui_vertexes_data + offsetof(struct _ui_Vertex, st)
  , .count = UI_NUM_VERTEXES
  , .stride = sizeof(_ui_vertexes_data[0])
  , .type_format = alias_memory_Format_Float32
  , .type_length = 2
};
static alias_ui_OutputGroup _ui_groups[UI_NUM_GROUPS];
static bool _ui_recording;

static alias_R _physics_speed;

static struct State * _current_state;

static uint32_t _screen_width;
static uint32_t _screen_height;

void Engine_init(uint32_t screen_width, uint32_t screen_height, const char * title, struct State * initial_state) {
  _screen_width = screen_width;
  _screen_height = screen_height;

  InitWindow(screen_width, screen_height, title);
  SetExitKey('Q');

#if USE_UV
  uv_loop_init(&_loop);
  _loop_mode = UV_RUN_NOWAIT;
  _running = true;

  uv_timer_init(&_loop, &_frame_timer);
  uv_timer_start(&_frame_timer, _frame_timer_f, 0, 16);
#else
  SetTargetFPS(60);
#endif

  alias_ecs_create_instance(NULL, &_ecs);

  alias_ui_initialize(alias_default_MemoryCB(), &_ui);
  _ui_recording = false;

  _physics_speed = alias_R_ONE;
  _current_state = NULL;

  Engine_push_state(initial_state);
}

static void _update_input(void);
static void _update_events(void);
static bool _update_state(void);
static void _update_physics(void);
static void _update_display(void);

static bool _update(void) {
  _update_physics();
  _update_display();
  if(WindowShouldClose()) {
    return false;
  }
  _update_input();
  _update_events();
  return _update_state();
}

#if USE_UV
bool Engine_update(void) {
  if(uv_run(&_loop, _loop_mode) == 0) {
    _running = false;
  }
  return _running;
}
static inline void _frame_timer_f(uv_timer_t * t) {
  if(!_update()) {
    _running = false;
  }

  if(_running) {
    uv_timer_again(t);
    _loop_mode = UV_RUN_ONCE;
  } else {
    uv_timer_stop(&_frame_timer);
    _loop_mode = UV_RUN_NOWAIT;
  }
}
#else
bool Engine_update(void) {
  return _update();
}
#endif

// state
void Engine_push_state(struct State * state) {
  if(_current_state != NULL && _current_state->pause != NULL) {
    _current_state->pause(_current_state->ud);
  }
  state->prev = _current_state;
  _current_state = state;
  if(state->begin != NULL) {
    state->begin(state->ud);
  }
}

void Engine_pop_state(void) {
  if(_current_state == NULL) {
    return;
  }
  if(_current_state->end != NULL) {
    _current_state->end(_current_state->ud);
  }
  _current_state = _current_state->prev;
  if(_current_state != NULL && _current_state->unpause != NULL) {
    _current_state->unpause(_current_state->ud);
  }
}

static bool _update_state(void) {
  struct State * current = _current_state;
  if(current == NULL) {
    return false;
  }
  if(current != NULL && current->frame != NULL) {
    current->frame(current->ud);
  }
  while(current != NULL) {
    if(current->background != NULL) {
      current->background(current->ud);
    }
    current = current->prev;
  }
  return true;
}

// parameter access
alias_ecs_Instance * Engine_ecs(void) {
  return _ecs;
}

// input
static uint32_t _input_backend_pair_count = 0;
static const struct InputBackendPair * _input_backend_pairs = NULL;

static uint32_t _input_binding_count = 0;
static alias_R * _input_bindings = NULL;

void Engine_set_player_input_backend(uint32_t player_index, uint32_t pair_count, const struct InputBackendPair * pairs) {
  (void)player_index;
  _input_backend_pair_count = pair_count;
  _input_backend_pairs = pairs;

  uint32_t max_binding_index = 0;
  for(uint32_t i = 0; i < pair_count; i++) {
    if(pairs[i].binding > max_binding_index) {
      max_binding_index = pairs[i].binding;
    }
  }

  if(max_binding_index + 1 > _input_binding_count) {
    _input_bindings = alias_realloc(
        alias_default_MemoryCB()
      , _input_bindings
      , sizeof(*_input_bindings) * _input_binding_count
      , sizeof(*_input_bindings) * (max_binding_index + 1)
      , alignof(*_input_bindings)
      );
    _input_binding_count = max_binding_index + 1;
  }
}

static struct {
  uint32_t count;
  union InputSignal * * signals;
} _input_frontends[MAX_INPUT_FRONTEND_SETS] = { 0 };

uint32_t Engine_add_input_frontend(uint32_t player_index, uint32_t signal_count, union InputSignal * * signals) {
  (void)player_index;

  uint32_t index = 0;
  for(; index < MAX_INPUT_FRONTEND_SETS; index++) {
    if(_input_frontends[index].count == 0) {
      break;
    }
  }
  if(index < MAX_INPUT_FRONTEND_SETS) {
    _input_frontends[index].count = signal_count;
    _input_frontends[index].signals = signals;
    return index;
  }
  return -1;
}

void Engine_remove_input_frontend(uint32_t player_index, uint32_t index) {
  (void)player_index;

  if(index < MAX_INPUT_FRONTEND_SETS) {
    _input_frontends[index].count = 0;
    _input_frontends[index].signals = NULL;
  }
}

static void _update_input(void) {
  static const int _to_raylib[] = {
    [Keyboard_Apostrophe] =   KEY_APOSTROPHE,
    [Keyboard_Comma] =        KEY_COMMA,
    [Keyboard_Minus] =        KEY_MINUS,
    [Keyboard_Period] =       KEY_PERIOD,
    [Keyboard_Slash] =        KEY_SLASH,
    [Keyboard_0] =            KEY_ZERO,
    [Keyboard_1] =            KEY_ONE,
    [Keyboard_2] =            KEY_TWO,
    [Keyboard_3] =            KEY_THREE,
    [Keyboard_4] =            KEY_FOUR,
    [Keyboard_5] =            KEY_FIVE,
    [Keyboard_6] =            KEY_SIX,
    [Keyboard_7] =            KEY_SEVEN,
    [Keyboard_8] =            KEY_EIGHT,
    [Keyboard_9] =            KEY_NINE,
    [Keyboard_Semicolon] =    KEY_SEMICOLON,
    [Keyboard_Equal] =        KEY_EQUAL,
    [Keyboard_A] =            KEY_A,
    [Keyboard_B] =            KEY_B,
    [Keyboard_C] =            KEY_C,
    [Keyboard_D] =            KEY_D,
    [Keyboard_E] =            KEY_E,
    [Keyboard_F] =            KEY_F,
    [Keyboard_G] =            KEY_G,
    [Keyboard_H] =            KEY_H,
    [Keyboard_I] =            KEY_I,
    [Keyboard_J] =            KEY_J,
    [Keyboard_K] =            KEY_K,
    [Keyboard_L] =            KEY_L,
    [Keyboard_M] =            KEY_M,
    [Keyboard_N] =            KEY_N,
    [Keyboard_O] =            KEY_O,
    [Keyboard_P] =            KEY_P,
    [Keyboard_Q] =            KEY_Q,
    [Keyboard_R] =            KEY_R,
    [Keyboard_S] =            KEY_S,
    [Keyboard_T] =            KEY_T,
    [Keyboard_U] =            KEY_U,
    [Keyboard_V] =            KEY_V,
    [Keyboard_W] =            KEY_X,
    [Keyboard_X] =            KEY_W,
    [Keyboard_Y] =            KEY_Y,
    [Keyboard_Z] =            KEY_Z,
    [Keyboard_Space] =        KEY_SPACE,
    [Keyboard_Escape] =       KEY_ESCAPE,
    [Keyboard_Enter] =        KEY_ENTER,
    [Keyboard_Tab] =          KEY_TAB,
    [Keyboard_Backspace] =    KEY_BACKSPACE,
    [Keyboard_Insert] =       KEY_INSERT,
    [Keyboard_Delete] =       KEY_DELETE,
    [Keyboard_Right] =        KEY_RIGHT,
    [Keyboard_Left] =         KEY_LEFT,
    [Keyboard_Down] =         KEY_DOWN,
    [Keyboard_Up] =           KEY_UP,
    [Keyboard_PageUp] =       KEY_PAGE_UP,
    [Keyboard_PageDown] =     KEY_PAGE_DOWN,
    [Keyboard_Home] =         KEY_HOME,
    [Keyboard_End] =          KEY_END,
    [Keyboard_CapsLock] =     KEY_CAPS_LOCK,
    [Keyboard_ScrollLock] =   KEY_SCROLL_LOCK,
    [Keyboard_NumLock] =      KEY_NUM_LOCK,
    [Keyboard_PrintScreen] =  KEY_PRINT_SCREEN,
    [Keyboard_Pause] =        KEY_PAUSE,
    [Keyboard_F1] =           KEY_F1,
    [Keyboard_F2] =           KEY_F2,
    [Keyboard_F3] =           KEY_F3,
    [Keyboard_F4] =           KEY_F4,
    [Keyboard_F5] =           KEY_F5,
    [Keyboard_F6] =           KEY_F6,
    [Keyboard_F7] =           KEY_F7,
    [Keyboard_F8] =           KEY_F8,
    [Keyboard_F9] =           KEY_F9,
    [Keyboard_F10] =          KEY_F10,
    [Keyboard_F11] =          KEY_F11,
    [Keyboard_F12] =          KEY_F12,
    [Keyboard_LeftShift] =    KEY_LEFT_SHIFT,
    [Keyboard_LeftControl] =  KEY_LEFT_CONTROL,
    [Keyboard_Leftalt] =      KEY_LEFT_ALT,
    [Keyboard_LeftMeta] =     KEY_LEFT_SUPER,
    [Keyboard_RightShift] =   KEY_RIGHT_SHIFT,
    [Keyboard_RightControl] = KEY_RIGHT_CONTROL,
    [Keyboard_RightAlt] =     KEY_RIGHT_ALT,
    [Keyboard_RightMeta] =    KEY_RIGHT_SUPER,
    [Keyboard_Menu] =         KEY_KB_MENU,
    [Keyboard_LeftBracket] =  KEY_LEFT_BRACKET,
    [Keyboard_Backslash] =    KEY_BACKSLASH,
    [Keyboard_RightBracket] = KEY_RIGHT_BRACKET,
    [Keyboard_Grave] =        KEY_GRAVE,
    [Keyboard_Pad_0] =        KEY_KP_0,
    [Keyboard_Pad_1] =        KEY_KP_1,
    [Keyboard_Pad_2] =        KEY_KP_2,
    [Keyboard_Pad_3] =        KEY_KP_3,
    [Keyboard_Pad_4] =        KEY_KP_4,
    [Keyboard_Pad_5] =        KEY_KP_5,
    [Keyboard_Pad_6] =        KEY_KP_6,
    [Keyboard_Pad_7] =        KEY_KP_7,
    [Keyboard_Pad_8] =        KEY_KP_8,
    [Keyboard_Pad_9] =        KEY_KP_9,
    [Keyboard_Pad_Period] =   KEY_KP_DECIMAL,
    [Keyboard_Pad_Divide] =   KEY_KP_DIVIDE,
    [Keyboard_Pad_Multiply] = KEY_KP_MULTIPLY,
    [Keyboard_Pad_Minus] =    KEY_KP_SUBTRACT,
    [Keyboard_Pad_Add] =      KEY_KP_ADD,
    [Keyboard_Pad_Enter] =    KEY_KP_ENTER,
    [Keyboard_Pad_Equal] =    KEY_KP_EQUAL,
    [Mouse_Left_Button] =     MOUSE_LEFT_BUTTON,
    [Mouse_Right_Button] =    MOUSE_RIGHT_BUTTON
  };

  alias_memory_clear(_input_bindings, sizeof(*_input_bindings) * _input_binding_count);

  for(uint32_t i = 0; i < _input_backend_pair_count; i++) {
    enum InputSource source = _input_backend_pairs[i].source;
    alias_R * binding = &_input_bindings[_input_backend_pairs[i].binding];

    switch(source) {
    case Keyboard_Apostrophe ... Keyboard_Pad_Equal:
      if(IsKeyDown(_to_raylib[source])) {
        *binding = alias_R_ONE;
      }
      break;
    case Mouse_Left_Button ... Mouse_Right_Button:
      if(IsMouseButtonDown(_to_raylib[source])) {
        *binding = alias_R_ONE;
      }
      break;
    case Mouse_Position_X:
      *binding = GetMouseX();
      break;
    case Mouse_Position_Y:
      *binding = GetMouseY();
      break;
    case InputSource_COUNT:
      break;
    }
  }

  for(uint32_t i = 0; i < MAX_INPUT_FRONTEND_SETS; i++) {
    for(uint32_t j = 0; j < _input_frontends[i].count; j++) {
      union InputSignal * signal = _input_frontends[i].signals[j];
      switch(signal->type) {
      case InputSignal_Pass:
        {
          signal->pass.value = _input_bindings[signal->pass.binding] > alias_R_ZERO;
          break;
        }
      case InputSignal_Up:
        {
          bool value = _input_bindings[signal->up.binding] > alias_R_ZERO;
          signal->up.value = !signal->up._internal_1 && value;
          signal->up._internal_1 = value;
          break;
        }
      case InputSignal_Down:
        {
          bool value = _input_bindings[signal->up.binding] > alias_R_ZERO;
          signal->up.value = signal->up._internal_1 && !value;
          signal->up._internal_1 = value;
          break;
        }
      case InputSignal_Direction:
        {
          signal->direction.value = alias_pga2d_direction(_input_bindings[signal->direction.binding_x], _input_bindings[signal->direction.binding_y]);
          break;
        }
      case InputSignal_Point:
        {
          signal->point.value = alias_pga2d_point(_input_bindings[signal->point.binding_x], _input_bindings[signal->point.binding_y]);
          break;
        }
      }
    }
  }
}

// events
DEFINE_COMPONENT(Event)

static uint32_t _events_id = 1;

uint32_t Engine_next_event_id(void) {
  return _events_id++;
}

QUERY( _update_events
  , state(uint32_t, last_highest_seen)
  , state(struct CmdBuf, cbuf)
  , state(uint32_t, next_highest_seen)
  , pre(
    CmdBuf_begin_recording(&state->cbuf);
    state->next_highest_seen = state->last_highest_seen;
  )
  , read(Event, e)
  , action(
    if(e->id <= state->last_highest_seen) {
      CmdBuf_despawn(&state->cbuf, entity);
    } else if(e->id > state->next_highest_seen) {
      state->next_highest_seen = e->id;
    }
  )
  , post(
    state->last_highest_seen = state->next_highest_seen;
    CmdBuf_end_recording(&state->cbuf);
    CmdBuf_execute(&state->cbuf, Engine_ecs());
  )
)

// transform
alias_TransformBundle * Engine_transform_bundle(void) {
  static alias_TransformBundle bundle;
  static bool init = false;
  if(!init) {
    alias_TransformBundle_initialize(Engine_ecs(), &bundle);
    init = true;
  }
  return &bundle;
}

// physics
alias_Physics2DBundle * Engine_physics_2d_bundle(void) {
  static alias_Physics2DBundle bundle;
  static bool init = false;
  if(!init) {
    alias_Physics2DBundle_initialize(Engine_ecs(), &bundle, Engine_transform_bundle());
    init = true;
  }
  return &bundle;
}

alias_R Engine_physics_speed(void) {
  return _physics_speed;
}

void Engine_set_physics_speed(alias_R speed) {
  _physics_speed = speed;
}

alias_R Engine_time(void) {
  return GetTime();
}

static void _update_physics(void) {
  const float timestep = 1.0f / 60.0f;

  static float p_time = 0.0f;
  static float s_time = 0.0f;

  s_time += GetFrameTime() * _physics_speed;

  if(p_time >= s_time) {
    alias_transform_update2d_serial(Engine_ecs(), Engine_transform_bundle());
  }

  while(p_time < s_time) {
    alias_physics_update2d_serial_pre_transform(Engine_ecs(), Engine_physics_2d_bundle(), timestep);

    alias_transform_update2d_serial(Engine_ecs(), Engine_transform_bundle());

    alias_physics_update2d_serial_post_transform(Engine_ecs(), Engine_physics_2d_bundle(), timestep);

    p_time += timestep;
  }
}

// render
DEFINE_COMPONENT(Camera)

DEFINE_COMPONENT(DrawRectangle)

DEFINE_COMPONENT(DrawCircle)

DEFINE_COMPONENT(DrawText)

static void _update_ui(void);

QUERY(_draw_rectangles
  , read(alias_LocalToWorld2D, t)
  , read(DrawRectangle, r)
  , action(
    alias_R
        hw = r->width / 2
      , hh = r->height / 2
      , bl = -hw
      , br =  hw
      , bt = -hh
      , bb =  hh
      ;

    alias_pga2d_Point box[] = {
        alias_pga2d_point(br, bb)
      , alias_pga2d_point(br, bt)
      , alias_pga2d_point(bl, bt)
      , alias_pga2d_point(bl, bb)
      };

    box[0] = alias_pga2d_sandwich_bm(box[0], t->motor);
    box[1] = alias_pga2d_sandwich_bm(box[1], t->motor);
    box[2] = alias_pga2d_sandwich_bm(box[2], t->motor);
    box[3] = alias_pga2d_sandwich_bm(box[3], t->motor);

    Vector2 points[] = {
      { alias_pga2d_point_x(box[0]), alias_pga2d_point_y(box[0]) },
      { alias_pga2d_point_x(box[1]), alias_pga2d_point_y(box[1]) },
      { alias_pga2d_point_x(box[2]), alias_pga2d_point_y(box[2]) },
      { alias_pga2d_point_x(box[3]), alias_pga2d_point_y(box[3]) },
    };

    DrawTriangle(points[0], points[1], points[2], alias_Color_to_raylib_Color(r->color));
    DrawTriangle(points[0], points[2], points[3], alias_Color_to_raylib_Color(r->color));
  )
)

QUERY(_draw_circles
  , read(alias_LocalToWorld2D, transform)
  , read(DrawCircle, c)
  , action(
    DrawCircle(alias_pga2d_point_x(transform->position), alias_pga2d_point_y(transform->position), c->radius, alias_Color_to_raylib_Color(c->color));
  )
)

QUERY(_draw_text
  , read(alias_LocalToWorld2D, w)
  , read(DrawText, t)
  , action(
    DrawText(t->text, alias_pga2d_point_x(w->position), alias_pga2d_point_y(w->position), t->size, alias_Color_to_raylib_Color(t->color));
  )
)

QUERY(_update_display
  , read(alias_LocalToWorld2D, transform)
  , read(Camera, camera)
  , pre(
    BeginDrawing();

    ClearBackground(alias_Color_to_raylib_Color(alias_Color_RAYWHITE));
  )
  , action(
    int x = alias_pga2d_point_x(camera->viewport_min) * _screen_width;
    int y = alias_pga2d_point_y(camera->viewport_min) * _screen_height;
    int width = (alias_pga2d_point_x(camera->viewport_max) * _screen_width) - x;
    int height = (alias_pga2d_point_y(camera->viewport_max) * _screen_height) - y;

    //BeginScissorMode(x, y, width, height);

    // thse numbers make no sense, why have this 'offset' bs
    BeginMode2D((Camera2D) {
      .offset = { width / 2.0f, height / 2.0f },                // Camera offset (displacement from target)
      .target = { alias_pga2d_point_x(transform->position), alias_pga2d_point_y(transform->position) }, // Camera target (rotation and zoom origin)
      .rotation = 0.0f,                                         // Camera rotation in degrees
      .zoom = camera->zoom                                      // Camera zoom (scaling), should be 1.0f by default
    });

    _draw_rectangles();
    _draw_circles();
    _draw_text();

    EndMode2D();
    //EndScissorMode();
  )
  , post(
    _update_ui();
    EndDrawing();
  )
)

// ====================================================================================================================
// Resource ===========================================================================================================
enum ResourceType {
  ResourceType_Image
};

struct LoadedResource {
  alias_InlineList list;

  enum ResourceType type;
  uint32_t id, gen;

  union {
    struct {
      Texture2D raylib;
      uint32_t width;
      uint32_t height;
    } image;
  };
};

alias_InlineList _free_resources = ALIAS_INLINE_LIST_INIT(_free_resources);
alias_InlineList _inactive_resources = ALIAS_INLINE_LIST_INIT(_inactive_resources);
alias_InlineList _active_resources = ALIAS_INLINE_LIST_INIT(_active_resources);

static uint32_t _resource_id = 1, _resource_gen = 1;

void _free_resource(struct LoadedResource * resource) {
  switch(resource->type) {
  case ResourceType_Image:
    UnloadTexture(resource->image.raylib);
    break;
  }

  alias_InlineList_remove_self(&resource->list);
  alias_InlineList_append(&_free_resources, &resource->list);
}

#define ALIAS_INLINE_LIST_EACH(LIST, ITEM) \
  for(ITEM = (LIST)->next; ITEM != (LIST); ITEM = ITEM->next)

#define ALIAS_INLINE_LIST_EACH_CONTAINER(LIST, ITEM, MEMBER)                         \
  for( ITEM = ALIAS_INLINE_LIST_CONTAINER((LIST)->next, typeof(*ITEM), MEMBER)       \
     ; &ITEM->MEMBER != (LIST)                                                       \
     ; ITEM = ALIAS_INLINE_LIST_CONTAINER(ITEM->MEMBER.next, typeof(*ITEM), MEMBER) \
     )

#define ALIAS_INLINE_LIST_EACH_CONTAINER_SAFE(LIST, ITEM, NEXT, MEMBER)              \
  for( ITEM = ALIAS_INLINE_LIST_CONTAINER((LIST)->next, typeof(*ITEM), MEMBER)       \
     , NEXT = ALIAS_INLINE_LIST_CONTAINER(ITEM->MEMBER.next, typeof(*ITEM), MEMBER) \
     ; &ITEM->MEMBER != (LIST)                                                       \
     ; ITEM = NEXT                                                                   \
     , NEXT = ALIAS_INLINE_LIST_CONTAINER(NEXT->MEMBER.next, typeof(*ITEM), MEMBER) \
     )

void _resources_gc(void) {
  struct LoadedResource * resource, * next_resource;

  ALIAS_INLINE_LIST_EACH_CONTAINER_SAFE(&_inactive_resources, resource, next_resource, list) {
    _free_resource(resource);
  }

  _resource_gen++;
}

struct LoadedResource * _allocate_resource(enum ResourceType type) {
  if(alias_InlineList_is_empty(&_free_resources)) {
    _resources_gc();
  }
  struct LoadedResource * resource;
  if(alias_InlineList_is_empty(&_free_resources)) {
    resource = alias_malloc(alias_default_MemoryCB(), sizeof(*resource), alignof(*resource));
    alias_InlineList_init(&resource->list);
  } else {
    resource = alias_InlineList_pop(&_free_resources);
  }
  alias_InlineList_append(&_active_resources, &resource->list);
  resource->type = type;
  resource->id = _resource_id++;
  resource->gen = _resource_gen;
  return resource;
}

void _touch_resource(struct LoadedResource * resource) {
  if(resource->gen != _resource_gen) {
    alias_InlineList_remove_self(&resource->list);
    alias_InlineList_append(&_active_resources, &resource->list);
    resource->gen = _resource_gen;
  }
}

struct LoadedResource * _load_image(struct Image * img) {
  struct LoadedResource * resource;

  if(img->resource == NULL || img->resource_id != img->resource->id) {
    char path[1024];
    snprintf(path, sizeof(path), "assets/%s", img->path);

    resource = _allocate_resource(ResourceType_Image);
    resource->image.raylib = LoadTexture(path);
    resource->image.width = resource->image.raylib.width;
    resource->image.height = resource->image.raylib.height;
    img->resource = resource;
    img->resource_id = resource->id;
  } else {
    _touch_resource(img->resource);
  }

  return img->resource;
}

// ====================================================================================================================
// UI =================================================================================================================

static inline void _text_size(const char * buffer, float size, float max_width, float * out_width, float * out_height) {
  (void)max_width;
  Vector2 dim = MeasureTextEx(GetFontDefault(), buffer, size, 0);
  *out_width = dim.x;
  *out_height = dim.y;
}

static inline void _text_draw(const char * buffer, float x, float y, float width, float size, alias_Color color) {
  (void)width;
  DrawText(buffer, x, y, size, alias_Color_to_raylib_Color(color));
}

static inline void _start_ui(void) {
  static alias_ui_Input input;

  if(!_ui_recording) {
    input.screen_size.width = _screen_width;
    input.screen_size.height = _screen_height;
    input.text_size = _text_size;
    input.text_draw = _text_draw;

    alias_ui_begin_frame(_ui, alias_default_MemoryCB(), &input);
    _ui_recording = true;

    alias_ui_begin_stack(_ui);
  }
}

void Engine_ui_image(struct Image * img) {
  _start_ui();
  struct LoadedResource * resource = _load_image(img);
  alias_ui_image(_ui, resource->image.width, resource->image.height, 0, 0, 1, 1, resource->image.raylib.id);
}

void Engine_ui_align_fractions(float x, float y) {
  _start_ui();
  alias_ui_align_fractions(_ui, x, y);
}

void Engine_ui_font_size(alias_R size) {
  _start_ui();
  alias_ui_font_size(_ui, size);
}

void Engine_ui_font_color(alias_Color color) {
  _start_ui();
  alias_ui_font_color(_ui, color);
}

void Engine_ui_text(const char * format, ...) {
  _start_ui();

  va_list ap;
  va_start(ap, format);
  alias_ui_textv(_ui, format, ap);
  va_end(ap);
}

void Engine_ui_vertical(void) {
  _start_ui();
  alias_ui_begin_vertical(_ui);
}

void Engine_ui_horizontal(void) {
  _start_ui();
  alias_ui_begin_horizontal(_ui);
}

void Engine_ui_stack(void) {
  _start_ui();
  alias_ui_begin_stack(_ui);
}

void Engine_ui_end(void) {
  _start_ui();
  alias_ui_end(_ui);
}

static void _update_ui(void) {
  static alias_ui_Output output;

  output.num_groups = 0;
  output.max_groups = UI_NUM_GROUPS;
  output.groups = _ui_groups;
  output.num_indexes = 0;
  output.num_vertexes = 0;
  output.index_sub_buffer = _ui_indexes;
  output.xy_sub_buffer = _ui_vertexes_xy;
  output.rgba_sub_buffer = _ui_vertexes_rgba;
  output.st_sub_buffer = _ui_vertexes_st;

  if(_ui_recording) {
    alias_ui_end(_ui); // end stack
    alias_ui_end_frame(_ui, alias_default_MemoryCB(), &output);
    _ui_recording = false;

    for(uint32_t g = 0; g < output.num_groups; g++) {
      uint32_t length = _ui_groups[g].length;
      if(length == 0) {
        continue;
      }

      rlSetTexture(_ui_groups[g].texture_id);

      rlBegin(RL_QUADS);
      for(uint32_t i = _ui_groups[g].index, end = i + length; i < end; ) {
        for(uint32_t j = 0; j < 3; j++) {
          uint32_t index = _ui_indexes_data[i++];
          rlColor4f(
              _ui_vertexes_data[index].rgba[0]
            , _ui_vertexes_data[index].rgba[1]
            , _ui_vertexes_data[index].rgba[2]
            , _ui_vertexes_data[index].rgba[3]
            );
          rlTexCoord2f(
              _ui_vertexes_data[index].st[0]
            , _ui_vertexes_data[index].st[1]
            );
          rlVertex2f(
              _ui_vertexes_data[index].xy[0]
            , _ui_vertexes_data[index].xy[1]
            );
          if(j == 0) {
            rlTexCoord2f(
                _ui_vertexes_data[index].st[0]
              , _ui_vertexes_data[index].st[1]
              );
            rlVertex2f(
                _ui_vertexes_data[index].xy[0]
              , _ui_vertexes_data[index].xy[1]
              );
          }
        }
      }
      rlEnd();
    }

    rlSetTexture(0);
  }
}
