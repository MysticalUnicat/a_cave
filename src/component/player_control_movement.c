#include "../component.h"
#include "../state/local.h"

static void _init(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data) {
  struct PlayerControlMovement * player_control = (struct PlayerControlMovement *)data[0];

  struct PlayerInputs * inputs = alias_malloc(alias_default_MemoryCB(), sizeof(*inputs), alignof(*inputs));
  player_control->inputs = inputs;

  inputs->pause = INPUT_SIGNAL_UP(Binding_Pause);
  inputs->left = INPUT_SIGNAL_PASS(Binding_PlayerLeft);
  inputs->right = INPUT_SIGNAL_PASS(Binding_PlayerRight);
  inputs->up = INPUT_SIGNAL_PASS(Binding_PlayerUp);
  inputs->down = INPUT_SIGNAL_PASS(Binding_PlayerDown);

  player_control->input_index = Engine_add_input_frontend(player_control->player_index, 5, &inputs->pause);
}

static void _cleanup(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data) {
  struct PlayerControlMovement * player_control = *(struct PlayerControlMovement **)data[0];

  Engine_remove_input_frontend(player_control->player_index, player_control->input_index);

  alias_free(alias_default_MemoryCB(), player_control->inputs, sizeof(player_control->inputs), alignof(player_control->inputs));
}

COMPONENT(
    PlayerControlMovement
  , .num_required_components = 1
  , .required_components = (alias_ecs_ComponentHandle[]) { Movement_component() }
  , .init = { _init, NULL }
  , .cleanup = { _cleanup, NULL }
  )

