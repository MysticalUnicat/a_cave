#include "component.h"
#include "state/local.h"

static void _player_control_movement_init(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data) {
  struct PlayerControlMovement * player_control = (struct PlayerControlMovement *)data[0];

  struct PlayerInputs * inputs = alias_malloc(alias_default_MemoryCB(), sizeof(*inputs), alignof(*inputs));
  player_control->inputs = inputs;

  inputs->pause = INPUT_SIGNAL_UP(Binding_Pause);
  inputs->left = INPUT_SIGNAL_PASS(Binding_PlayerLeft);
  inputs->right = INPUT_SIGNAL_PASS(Binding_PlayerRight);
  inputs->up = INPUT_SIGNAL_PASS(Binding_PlayerUp);
  inputs->down = INPUT_SIGNAL_PASS(Binding_PlayerDown);
  inputs->mouse_position = INPUT_SIGNAL_VIEWPORT_POINT(Binding_MouseX, Binding_MouseY, 0);
  inputs->mouse_left_up = INPUT_SIGNAL_UP(Binding_LeftClick);
  inputs->mouse_left_down = INPUT_SIGNAL_DOWN(Binding_LeftClick);
  inputs->mouse_right_up = INPUT_SIGNAL_UP(Binding_RightClick);
  inputs->mouse_right_down = INPUT_SIGNAL_DOWN(Binding_RightClick);

  player_control->input_index = Engine_add_input_frontend(player_control->player_index, 10, &inputs->pause);
}

static void _player_control_movement_cleanup(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data) {
  struct PlayerControlMovement * player_control = *(struct PlayerControlMovement **)data[0];

  Engine_remove_input_frontend(player_control->player_index, player_control->input_index);

  alias_free(alias_default_MemoryCB(), player_control->inputs, sizeof(player_control->inputs), alignof(player_control->inputs));
}

COMPONENT(
    PlayerControlMovement
  , .num_required_components = 1
  , .required_components = (alias_ecs_ComponentHandle[]) { Movement_component() }
  , .init = { _player_control_movement_init, NULL }
  , .cleanup = { _player_control_movement_cleanup, NULL }
  )

DEFINE_COMPONENT(
    Movement
  , .num_required_components = 1
  , .required_components = (alias_ecs_ComponentHandle[]) { alias_Physics2DBodyMotion_component() }
  )

DEFINE_COMPONENT(Shield)

DEFINE_COMPONENT(Armor)

DEFINE_COMPONENT(Power)

