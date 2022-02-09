#include "state/local.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

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

struct MainInputs main_inputs = {
    .menu_up = INPUT_SIGNAL_UP(Binding_PlayerUp)
  , .menu_down = INPUT_SIGNAL_UP(Binding_PlayerDown)
  , .menu_left = INPUT_SIGNAL_UP(Binding_PlayerLeft)
  , .menu_right = INPUT_SIGNAL_UP(Binding_PlayerRight)
  , .menu_back = INPUT_SIGNAL_UP(Binding_Back)
  , .menu_forward = INPUT_SIGNAL_UP(Binding_Forward)
  , .mouse_position = INPUT_SIGNAL_POINT(Binding_MouseX, Binding_MouseY)
  , .mouse_left_click = INPUT_SIGNAL_DOWN(Binding_LeftClick)
  };

extern struct State intro_state;

int main(void) {
  Engine_init(SCREEN_WIDTH, SCREEN_HEIGHT, "alias town", &intro_state);

  Engine_set_player_input_backend(0, sizeof(main_input_backend) / sizeof(main_input_backend[0]), main_input_backend);

  Engine_add_input_frontend(0, 8, &main_inputs.menu_up);

  Engine_run();
}
