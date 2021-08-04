
  XX(RightMeta,       RIGHT_SUPER) \
  XX(Menu,            KB_MENU) \
  XX(LeftBracket,     LEFT_BRACKET  ) \
  XX(Backslash,       BACKSLASH) \
  XX(RightBracket,    RIGHT_BRACKET ) \
  XX(Grave,           GRAVE) \
  XX(Pad_0,           KP_0) \
  XX(Pad_1,           KP_1) \
  XX(Pad_2,           KP_2) \
  XX(Pad_3,           KP_3) \
  XX(Pad_4,           KP_4) \
  XX(Pad_5,           KP_5) \
  XX(Pad_6,           KP_6) \
  XX(Pad_7,           KP_7) \
  XX(Pad_8,           KP_8) \
  XX(Pad_9,           KP_9) \
  XX(Pad_Period,      KP_DECIMAL) \
  XX(Pad_Divide,      KP_DIVIDE) \
  XX(Pad_Multiply,    KP_MULTIPLY) \
  XX(Pad_Minus,       KP_SUBTRACT) \
  XX(Pad_Add,         KP_ADD) \
  XX(Pad_Enter,       KP_ENTER) \
  XX(Pad_Equal,       KP_EQUAL)

#define __KEYS_ENUM(MINE, RAYLIB) Key_ ## MINE ,
#define __KEYS_FROM_RAYLIB(MINE, RAYLIB) [KEY_ ## RAYLIB] = Key_ ## MINE ,
#define __KEYS_TO_RAYLIB(MINE, RAYLIB) [Key_ ## MINE] = KEY_ ## RAYLIB ,

enum Key {
  __KEYS(__KEYS_ENUM)

  Key_FIRST = Key_Apostrophe,
  Key_LAST  = Key_Pad_Equal,
  Key_COUNT = Key_LAST - Key_FIRST
};
enum Key _raylib_to_key[] = {
  __KEYS(__KEYS_FROM_RAYLIB)
};
int _key_to_raylib[] = {
  __KEYS(__KEYS_TO_RAYLIB)
};

enum MouseButton {
    MouseButton_Left
  , MouseButton_Right

  , MouseButton_FIRST = MouseButton_Left
  , MouseButton_LAST  = MouseButton_Right
  , MouseButton_COUNT = MouseButton_LAST - MouseButton_FIRST
};
enum MouseButton _raylib_to_mouse_button[] = {
    [MOUSE_LEFT_BUTTON] = MouseButton_Left
  , [MOUSE_RIGHT_BUTTON] = MouseButton_Right
};
int _mouse_button_to_raylib[] = {
    [MouseButton_Left] = MOUSE_LEFT_BUTTON
  , [MouseButton_Right] = MOUSE_RIGHT_BUTTON
};

enum InputBinding {
    InputBinding_None
  , InputBinding_Back
  , InputBinding_Forward
  , InputBinding_Pause
  , InputBinding_LeftClick
  , InputBinding_RightClick
  , InputBinding_COUNT
};

alias_R bindings[InputBinding_COUNT];

struct {
  enum InputBinding keyboard_bindings[Key_COUNT];
  enum InputBinding mouse_button_bindings[MouseButton_COUNT];
  bool menu_back;
  bool menu_forward;
  bool pause;
  bool left_click;
  bool right_click;
  alias_Vector2D mouse_position;
} Input;

void input_init(void) {
  alias_memory_clear(Input.keyboard_bindings, sizeof(Input.keyboard_bindings));
  alias_memory_clear(Input.mouse_button_bindings, sizeof(Input.mouse_button_bindings));

  Input.keyboard_bindings[Key_Escape] = InputBinding_Back;
  Input.keyboard_bindings[Key_Enter]  = InputBinding_Forward;
  Input.keyboard_bindings[Key_P]      = InputBinding_Pause;
  Input.mouse_button_bindings[MouseButton_Left] = InputBinding_LeftClick;
  Input.mouse_button_bindings[MouseButton_Right] = InputBinding_RightClick;

  SetExitKey('Q');
}

void input_update(void) {
  alias_memory_clear(bindings, sizeof(bindings));
  
  for(enum Key key = Key_FIRST; key <= Key_LAST; key++) {
    enum InputBinding binding = Input.keyboard_bindings[key];

    if(binding == InputBinding_None) {
      continue;
    }
    
    if(IsKeyDown(_key_to_raylib[key])) {
      bindings[binding] = alias_R_ONE;
    }
  }

  for(enum MouseButton button = MouseButton_FIRST; button <= MouseButton_LAST; button++) {
    enum InputBinding binding = Input.mouse_button_bindings[button];

    if(binding == InputBinding_None) {
      continue;
    }
    
    if(IsMouseButtonPressed(_mouse_button_to_raylib[button])) {
      bindings[binding] = alias_R_ONE;
    }
  }

#define __SIGNAL_UP(F, B) \
  do { \
    static bool F ## _state; \
    Input.F = !F ## _state && bindings[InputBinding_ ## B] > alias_R_ZERO; \
    F ## _state = bindings[InputBinding_ ## B] > alias_R_ZERO; \
  } while(0)

  __SIGNAL_UP(menu_back, Back);
  __SIGNAL_UP(menu_forward, Forward);
  __SIGNAL_UP(pause, Pause);
  __SIGNAL_UP(left_click, LeftClick);
  __SIGNAL_UP(right_click, RightClick);

  Input.mouse_position.x = GetMouseX();
  Input.mouse_position.y = GetMouseY();
}
