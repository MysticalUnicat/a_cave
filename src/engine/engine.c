#include "engine.h"

#include <raylib.h>

static alias_ecs_Instance * _ecs;
static alias_R _physics_speed;
static struct State * _current_state;

void Engine_init(uint32_t screen_width, uint32_t screen_height, const char * title, struct State * initial_state) {
  InitWindow(screen_width, screen_height, title);
  SetTargetFPS(60);
  SetExitKey('Q');

  alias_ecs_create_instance(NULL, &_ecs);

  _physics_speed = alias_R_ONE;
  _current_state = NULL;

  Engine_push_state(initial_state);
}

static void _update_input(void);
static void _update_events(void);
static bool _update_state(void);
static void _update_physics(void);
static void _update_display(void);

bool Engine_update(void) {
  _update_physics();
  _update_display();
  if(WindowShouldClose()) {
    return false;
  }

  _update_input();
  _update_events();

  return _update_state();
}

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

  if(max_binding_index > _input_binding_count) {
    _input_bindings = alias_realloc(
        alias_default_MemoryCB()
      , _input_bindings
      , sizeof(*_input_bindings) * _input_binding_count
      , sizeof(*_input_bindings) * max_binding_index
      , alignof(*_input_bindings)
      );
  }
}

static uint32_t _input_frontend_signal_count = 0;
static union InputSignal * * _input_frontend_signals = NULL;

void Engine_set_player_input_frontend(uint32_t player_index, uint32_t signal_count, union InputSignal * * signals) {
  (void)player_index;
  _input_frontend_signal_count = signal_count;
  _input_frontend_signals = signals;
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
    }
  }

  for(uint32_t i = 0; i < _input_frontend_signal_count; i++) {
    switch(_input_frontend_signals[i]->type) {
    case InputSignal_Up:
      {
        struct InputSignalUp * signal = &_input_frontend_signals[i]->up;
        bool value = _input_bindings[signal->binding] > alias_R_ZERO;
        signal->value = signal->_internal_1 == false && value;
        signal->_internal_1 = value;
        break;
      }
    case InputSignal_Vector2D:
      {
        struct InputSignalVector2D * signal = &_input_frontend_signals[i]->vector2d;
        signal->value.x = _input_bindings[signal->binding_x];
        signal->value.y = _input_bindings[signal->binding_y];
        break;
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

  last_highest_seen = next_highest_seen;

  CmdBuf_end_recording(&cbuf);
  CmdBuf_execute(&cbuf, Engine_ecs());
}

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
DEFINE_COMPONENT(DrawRectangle)

DEFINE_COMPONENT(DrawCircle)

DEFINE_COMPONENT(DrawText)

static void _update_hud(void);

static void _update_display(void) {
  BeginDrawing();

  ClearBackground(RAYWHITE);

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

    DrawTriangle(*(Vector2*)&box[0], *(Vector2*)&box[1], *(Vector2*)&box[2], r->color);
    DrawTriangle(*(Vector2*)&box[0], *(Vector2*)&box[2], *(Vector2*)&box[3], r->color);
  }

  QUERY(
      ( read, alias_LocalToWorld2D, t )
    , ( read, DrawCircle, c )
  ) {
    DrawCircle(t->value._13, t->value._23, c->radius, c->color);
  }

  QUERY(
      ( read, alias_LocalToWorld2D, w )
    , ( read, DrawText, t )
  ) {
    DrawText(t->text, w->value._13, w->value._23, t->size, t->color);
  }

  _update_hud();

  EndDrawing();
}

// hud
DEFINE_COMPONENT(HudTransform)

DEFINE_COMPONENT(HudAnchor)

DEFINE_COMPONENT(HudParent)

DEFINE_COMPONENT(HudText)

static void _update_hud(void) {
  
}
