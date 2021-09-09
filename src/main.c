#include "engine/engine.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

enum Binding {
  Binding_Back,
  Binding_Forward,
  Binding_Pause,
  Binding_LeftClick,
  Binding_RightClick,
  Binding_MouseX,
  Binding_MouseY,
  Binding_PlayerLeft,
  Binding_PlayerRight,
  Binding_PlayerUp,
  Binding_PlayerDown,
};

struct InputBackendPair main_input_backend[] = {
  { Keyboard_Enter, Binding_Forward },
  { Keyboard_Escape, Binding_Back },
  { Keyboard_P, Binding_Pause },
  { Mouse_Left_Button, Binding_LeftClick },
  { Mouse_Right_Button, Binding_RightClick },
  { Mouse_Position_X, Binding_MouseX },
  { Mouse_Position_Y, Binding_MouseY },
  { Keyboard_Left, Binding_PlayerLeft },
  { Keyboard_Right, Binding_PlayerRight },
  { Keyboard_Up, Binding_PlayerUp },
  { Keyboard_Down, Binding_PlayerDown },
  { Keyboard_A, Binding_PlayerLeft },
  { Keyboard_D, Binding_PlayerRight },
  { Keyboard_W, Binding_PlayerUp },
  { Keyboard_S, Binding_PlayerDown },
};

struct InputSignalUp menu_back = INPUT_SIGNAL_UP(Binding_Back);
struct InputSignalUp menu_forward = INPUT_SIGNAL_UP(Binding_Forward);
struct InputSignalVector2D mouse_position = INPUT_SIGNAL_VECTOR2D(Binding_MouseX, Binding_MouseY);
struct InputSignalDown mouse_left_click = INPUT_SIGNAL_DOWN(Binding_LeftClick);

union InputSignal * _main_signals[] = {
  (union InputSignal *)&menu_back,
  (union InputSignal *)&menu_forward,
  (union InputSignal *)&mouse_position
};

#include "intro.c"

int main(void) {
  Engine_init(SCREEN_WIDTH, SCREEN_HEIGHT, "a cave", &intro_state);

  Engine_set_player_input_backend(0, sizeof(main_input_backend) / sizeof(main_input_backend[0]), main_input_backend);

  Engine_add_input_frontend(0, sizeof(_main_signals) / sizeof(_main_signals[0]), _main_signals);

  while(Engine_update()) {
  }
}
