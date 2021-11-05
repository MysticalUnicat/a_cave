#include "../component.h"

DEFINE_COMPONENT(
    Movement
  , .num_required_components = 1
  , .required_components = (alias_ecs_ComponentHandle[]) { alias_Physics2DBodyMotion_component() }
  )
