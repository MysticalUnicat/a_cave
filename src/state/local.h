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

extern struct InputSignalUp menu_back;
extern struct InputSignalUp menu_forward;
extern struct InputSignalPoint mouse_position;
extern struct InputSignalDown mouse_left_click;
