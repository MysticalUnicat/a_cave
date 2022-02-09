#pragma once

#include "../component.h"
#include "../system.h"

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

struct MainInputs {
  struct InputSignal menu_up;
  struct InputSignal menu_down;
  struct InputSignal menu_left;
  struct InputSignal menu_right;
  struct InputSignal menu_back;
  struct InputSignal menu_forward;
  struct InputSignal mouse_position;
  struct InputSignal mouse_left_click;
};

extern struct MainInputs main_inputs;
