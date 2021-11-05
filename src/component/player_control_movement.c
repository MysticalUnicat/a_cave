#include "../component.h"

#define COMPONENT_impl(IDENT, ITYPE, DREF, ...)                                                      \
  LAZY_GLOBAL(                                                                                       \
    alias_ecs_ComponentHandle,                                                                       \
    IDENT##_component,                                                                               \
    ECS(register_component, Engine_ecs()                                                             \
      , &(alias_ecs_ComponentCreateInfo) { .size = sizeof(ITYPE), ## __VA_ARGS__ }, &inner);         \
  )                                                                                                  \
  const struct IDENT * IDENT##_read(Entity entity) {                                                 \
    const ITYPE * ptr;                                                                               \
    alias_ecs_read_entity_component(Engine_ecs(), entity, IDENT##_component(), (const void **)&ptr); \
    return DREF ptr;                                                                                 \
  }                                                                                                  \
  struct IDENT * IDENT##_write(Entity entity) {                                                      \
    ITYPE * ptr;                                                                                     \
    alias_ecs_write_entity_component(Engine_ecs(), entity, IDENT##_component(), (void **)&ptr);      \
    return DREF ptr;                                                                                 \
  }

#define COMPONENT(IDENT, ...) COMPONENT_impl(IDENT, struct IDENT, , ## __VA_ARGS__)
#define COMPONENT_pinned(IDENT, ...) COMPONENT_impl(IDENT, struct IDENT *, *, ## __VA_ARGS__)

COMPONENT_pinned(
    PlayerControlMovement
  , .num_required_components = 1
  , .required_components = (alias_ecs_ComponentHandle[]) { Movement_component() }
  )
