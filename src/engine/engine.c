#include "engine.h"

#include <alias/ui.h>

#define USE_UV 1

// why does this not put things in a namespace?!
#define Image raylib_Image
#define Color raylib_Color
#include <raylib.h>
#undef Image
#undef Color

const struct alias_Color alias_Color_RAYWHITE = (struct alias_Color) { 0.96, 0.96, 0.96, 0.96 };

static inline raylib_Color alias_Color_to_raylib_Color(struct alias_Color c) {
  return (raylib_Color) { c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0 };
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

  alias_ui_initilize(alias_default_MemoryCB(), &_ui);
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
      case InputSignal_Vector2D:
        {
          signal->vector2d.value.x = _input_bindings[signal->vector2d.binding_x];
          signal->vector2d.value.y = _input_bindings[signal->vector2d.binding_y];
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

#if 0
  void CAT(NAME, _do) \
    ( void * ud \
    , alias_ecs_Instance * instance \
    , alias_ecs_EntityHandle entity \
      MAP(_QUERY_wparam, __VA_ARGS__) \
      MAP(_QUERY_rparam, __VA_ARGS__) \
  ); \
  static void CAT(NAME, _do_wrap)(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data) { \
    uint32_t i = 0; \
    MAP(_QUERY_wext, __VA_ARGS__) \
    MAP(_QUERY_rext, __VA_ARGS__) \
    EACH \
    CAT(NAME, _do)(ud, instance, entity MAP(_QUERY_warg, __VA_ARGS__) MAP(_QUERY_rarg, __VA_ARGS__)); \
  } \
  void NAME(void) { \
    static alias_ecs_Query * query = NULL; \
    if(query == NULL) { \
      alias_ecs_ComponentHandle _rlist[] = { MAP(_QUERY_rlist, __VA_ARGS__) }; \
      alias_ecs_ComponentHandle _wlist[] = { MAP(_QUERY_wlist, __VA_ARGS__) }; \
      alias_ecs_QueryFilterCreateInfo _flist[] = { MAP(_QUERY_flist, __VA_ARGS__) }; \
      alias_ecs_create_query(Engine_ecs(), &(alias_ecs_QueryCreateInfo) { \
        .num_write_components = sizeof(_wlist) / sizeof(_wlist[0]), \
        .write_components = _wlist, \
        .num_read_components = sizeof(_rlist) / sizeof(_rlist[0]), \
        .read_components = _rlist, \
        .num_filters = sizeof(_flist) / sizeof(_flist[0]), \
        .filters = _flist \
      }, &query); \
    } \
    PRE \
    alias_ecs_execute_query(Engine_ecs(), query, (alias_ecs_QueryCB) { CAT(NAME, _do_wrap), NULL }); \
    POST \
  } \
  void CAT(NAME, _do) \
    ( void * ud \
    , alias_ecs_Instance * instance \
    , alias_ecs_EntityHandle entity \
      MAP(_QUERY_wparam, __VA_ARGS__) \
      MAP(_QUERY_rparam, __VA_ARGS__) \
  )
#endif

#include <alias/cpp.h>

#define ALIAS_EQ(PREFIX, X, Y) ALIAS__IS_PROBE(ALIAS__EQ__ ## PREFIX ## X ## __ ## PREFIX ## Y ## __)

#define ALIAS_EQ__Qstatic_var__Qstatic_var__ ALIAS__PROBE
#define ALIAS_EQ__Qpre__Qpre__               ALIAS__PROBE
#define ALIAS_EQ__Qpost__Qpost__             ALIAS__PROBE
#define ALIAS_EQ__Qread__Qread__             ALIAS__PROBE
#define ALIAS_EQ__Qitem__Qitem__             ALIAS__PROBE

#define Q(NAME, ...) ALIAS_EVAL(Q_impl(NAME, __VA_ARGS__))
#define Q_impl(NAME, ...) \
  static void ALIAS_CAT(NAME, _do)(alias_ecs_Instance * instance, alias_ecs_EntityHandle entity Q_do_arguments(__VA_ARGS__)) { \
  } \
  static void ALIAS_CAT(NAME, _do_wrap)(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data) { \
    ALIAS_CAT(NAME, _do)(instance, entity); \
  } \
  void NAME(void) { \
    static alias_ecs_Query * query = NULL; \
    alias_ecs_execute_query(Engine_ecs(), query, (alias_ecs_QueryCB) { ALIAS_CAT(NAME, _do_wrap), NULL }); \
  }
#define Q_do_arguments(...) \
  ALIAS_MAP(Q_do_arguments_static_var, ## __VA_ARGS__)

#define Q_do_arguments_static_var(X) ALIAS_IFF(ALIAS_EQ(

Q( _update_events
  , static_var(uint32_t, last_highest_seen, 0)
  , static_var(struct CmdBuf, cbuf)
  , pre(
    CmdBuf_begin_recording(&cbuf);
    uint32_t next_highest_seen = last_highest_seen;
  )
  , post(
    last_highest_seen = next_highest_seen;

    CmdBuf_end_recording(&cbuf);
    CmdBuf_execute(&cbuf, Engine_ecs());
  )
  , read(Event, e)
  , item(
    if(e->id <= last_highest_seen) {
      //STAT(cleanup ECS event);
      CmdBuf_despawn(&cbuf, entity);
    } else if(e->id > next_highest_seen) {
      next_highest_seen = e->id;
    }
  )
)

/*
static void _update_events(void) {
  static uint32_t last_highest_seen = 0;

  static struct CmdBuf cbuf;
  CmdBuf_begin_recording(&cbuf);

  uint32_t next_highest_seen = last_highest_seen;
  QUERY(( read, Event, e )) {
    if(e->id <= last_highest_seen) {
      //STAT(cleanup ECS event);
      CmdBuf_despawn(&cbuf, entity);
    } else if(e->id > next_highest_seen) {
      next_highest_seen = e->id;
    }
  }

  CmdBuf_end_recording(&cbuf);
  CmdBuf_execute(&cbuf, Engine_ecs());
}
*/

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

static void _update_display(void) {
  BeginDrawing();

  ClearBackground(alias_Color_to_raylib_Color(alias_Color_RAYWHITE));

  QUERY(
      ( read, alias_LocalToWorld2D, transform )
    , ( read, Camera, camera )
  ) {
    int x = camera->viewport_min.x * _screen_width;
    int y = camera->viewport_min.y * _screen_height;
    int width = (camera->viewport_max.x * _screen_width) - x;
    int height = (camera->viewport_max.y * _screen_height) - y;

    BeginScissorMode(x, y, width, height);

    BeginMode2D((Camera2D) {
      .offset = { 0.0f, 0.0f },                                 // Camera offset (displacement from target)
      .target = { transform->value._13, transform->value._23 }, // Camera target (rotation and zoom origin)
      .rotation = 0.0f,                                         // Camera rotation in degrees
      .zoom = camera->zoom                                      // Camera zoom (scaling), should be 1.0f by default
    });

    QUERY(
        ( read, alias_LocalToWorld2D, t )
      , ( read, DrawRectangle, r )
    ) {
      alias_R
          hw = r->width / 2
        , hh = r->height / 2
        , bl = -hw
        , br =  hw
        , bt = -hh
        , bb =  hh
        ;

      alias_Point2D box[] = {
          { br, bb }
        , { br, bt }
        , { bl, bt }
        , { bl, bb }
        };

      box[0] = alias_multiply_Affine2D_Point2D(t->value, box[0]);
      box[1] = alias_multiply_Affine2D_Point2D(t->value, box[1]);
      box[2] = alias_multiply_Affine2D_Point2D(t->value, box[2]);
      box[3] = alias_multiply_Affine2D_Point2D(t->value, box[3]);

      DrawTriangle(*(Vector2*)&box[0], *(Vector2*)&box[1], *(Vector2*)&box[2], Color_to_raylib_Color(r->color));
      DrawTriangle(*(Vector2*)&box[0], *(Vector2*)&box[2], *(Vector2*)&box[3], Color_to_raylib_Color(r->color));
    }

    QUERY(
        ( read, alias_LocalToWorld2D, t )
      , ( read, DrawCircle, c )
    ) {
      DrawCircle(t->value._13, t->value._23, c->radius, Color_to_raylib_Color(c->color));
    }

    QUERY(
        ( read, alias_LocalToWorld2D, w )
      , ( read, DrawText, t )
    ) {
      DrawText(t->text, w->value._13, w->value._23, t->size, Color_to_raylib_Color(t->color));
    }

    EndMode2D();
    EndScissorMode();
  }

  _update_ui();

  EndDrawing();
}

// ====================================================================================================================
// UI =================================================================================================================
static inline void _start_ui(void) {
  static alias_ui_Input input;

  if(!_ui_recording) {
    input.screen_size.width = _screen_width;
    input.screen_size.height = _screen_height;

    alias_ui_begin_frame(_ui, alias_default_MemoryCB(), &input);
    _ui_recording = true;
  }
}

void Engine_ui_center(void) {
  _start_ui();
  alias_ui_center(_ui);
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

static void _update_ui(void) {
  static alias_ui_Output output;

  if(_ui_recording) {
    alias_ui_end_frame(_ui, alias_default_MemoryCB(), &output);
    _ui_recording = false;
  }
}
