#include "engine/engine.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

enum Binding {
  Binding_Back,
  Binding_Forward,
  Binding_LeftClick,
  Binding_RightClick,
  Binding_MouseX,
  Binding_MouseY
};

struct InputBackendPair main_input_backend[] = {
  { Keyboard_Enter, Binding_Forward },
  { Keyboard_Escape, Binding_Back },
  { Mouse_Left_Button, Binding_LeftClick },
  { Mouse_Right_Button, Binding_RightClick },
  { Mouse_Position_X, Binding_MouseX },
  { Mouse_Position_Y, Binding_MouseY }
};

struct InputSignalUp menu_back = { .binding = Binding_Back };
struct InputSignalUp menu_forward = { .binding = Binding_Forward };
struct InputSignalVector2D mouse_position = { .binding_x = Binding_MouseX, .binding_y = Binding_MouseY };

#include "intro.c"

int main(void) {
  Engine_init(SCREEN_WIDTH, SCREEN_HEIGHT, "a cave", &intro_state);

  Engine_set_player_input_backend(0, sizeof(main_input_backend) / sizeof(main_input_backend[0]), main_input_backend);

  Engine_set_player_input_frontend(0, 3, (union InputSignal *[]){
    (union InputSignal *)&menu_back,
    (union InputSignal *)&menu_forward,
    (union InputSignal *)&mouse_position
  });

  while(Engine_update()) {
  }
}
