#ifndef _UTIL_H_
#define _UTIL_H_

#include <alias/ecs.h>
#include <raylib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "pp.h"

#define Entity alias_ecs_EntityHandle

#define LAZY_GLOBAL(TYPE, IDENT, ...) \
  TYPE IDENT (void) {                 \
    static TYPE inner;                \
    static int init = 0;              \
    if(init == 0) {                   \
      __VA_ARGS__                     \
      init = 1;                       \
    }                                 \
    return inner;                     \
  }

#define LAZY_GLOBAL_PTR(TYPE, IDENT, ...) \
  TYPE * IDENT (void) {                   \
    static TYPE inner;                    \
    static TYPE * __ptr = NULL;           \
    if(__ptr == NULL) {                   \
      __VA_ARGS__                         \
      __ptr = &inner;                     \
    }                                     \
    return __ptr;                         \
  }

#define ECS(F, ...) assert(ALIAS_ECS_SUCCESS == alias_ecs_##F(__VA_ARGS__))

#define DEFINE_FONT(IDENT, PATH) LAZY_GLOBAL_PTR(Font, IDENT, inner = LoadFont(PATH);)

#define DEFINE_WORLD(IDENT) LAZY_GLOBAL(alias_ecs_Instance *, IDENT, ECS(create_instance, NULL, &inner);)

#define DEFINE_COMPONENT(WORLD, IDENT, FIELDS)                                                                 \
  struct IDENT FIELDS;                                                                                         \
  LAZY_GLOBAL(                                                                                                 \
    alias_ecs_ComponentHandle,                                                                                 \
    IDENT##_component,                                                                                         \
    ECS(register_component, WORLD, &(alias_ecs_ComponentCreateInfo) { .size = sizeof(struct IDENT) }, &inner); \
  )

#define GET_COMPONENT(X) X ## _component (),

#define DEFINE_QUERY(WORLD, NAME, W_COUNT, ...)                   \
  LAZY_GLOBAL(alias_ecs_Query *, NAME,                            \
    alias_ecs_ComponentHandle handles[] = {                       \
      MAP(GET_COMPONENT, __VA_ARGS__)                            \
    };                                                            \
    uint32_t handle_count = sizeof(handles) / sizeof(handles[0]); \
    alias_ecs_create_query(WORLD, &(alias_ecs_QueryCreateInfo) {  \
      .num_write_components = W_COUNT,                            \
      .write_components = handles,                                \
      .num_read_components = handle_count - W_COUNT,              \
      .read_components = handles + W_COUNT,                       \
    }, &inner);                                                   \
  )

#define RUN_QUERY(WORLD, QUERY) \
  auto void CAT(query_fn_, __LINE__)(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data); \
  alias_ecs_execute_query(WORLD, QUERY, (alias_ecs_QueryCB) { CAT(query_fn_, __LINE__), NULL }); \
  auto void CAT(query_fn_, __LINE__)(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data)

#define SPAWN_COMPONENT(...) SPAWN_COMPONENT_ __VA_ARGS__
#define SPAWN_COMPONENT_(TYPE, ...) { .component = TYPE##_component(), .stride = sizeof(struct TYPE), .data = (void *)&(struct TYPE) { __VA_ARGS__ } },

#define SPAWN(WORLD, ...) ({                                        \
  alias_ecs_EntityHandle _entity;                                   \
  alias_ecs_EntitySpawnComponent _components[] = {                  \
    MAP(SPAWN_COMPONENT, __VA_ARGS__)                               \
  };                                                                \
  ECS(spawn, WORLD, &(alias_ecs_EntitySpawnInfo) {                  \
    .layer = ALIAS_ECS_INVALID_LAYER,                               \
    .count = 1,                                                     \
    .num_components = sizeof(_components) / sizeof(_components[0]), \
    .components = _components                                       \
  }, &_entity);                                                     \
  _entity;                                                          \
})

#define _QUERY_wlist(...)               _QUERY_wlist_ __VA_ARGS__               // unwrap
#define _QUERY_wlist_(KIND, ...)        CAT(_QUERY_wlist_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_wlist_write(TYPE, _NAME) TYPE##_component(),                    // if its write, emit getting the component for the type
#define _QUERY_wlist_read(...)

#define _QUERY_rlist(...)               _QUERY_rlist_ __VA_ARGS__               // unwrap
#define _QUERY_rlist_(KIND, ...)        CAT(_QUERY_rlist_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rlist_read(TYPE, _NAME)  TYPE##_component(),                    // if its read, emit getting the component for the type
#define _QUERY_rlist_write(...)

#define _QUERY_wparam(...)              _QUERY_wparam_ __VA_ARGS__              // unwrap
#define _QUERY_wparam_(KIND, ...)       CAT(_QUERY_wparam_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_wparam_write(TYPE, NAME) , struct TYPE * NAME
#define _QUERY_wparam_read(...)

#define _QUERY_rparam(...)              _QUERY_rparam_ __VA_ARGS__              // unwrap
#define _QUERY_rparam_(KIND, ...)       CAT(_QUERY_rparam_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rparam_read(TYPE, NAME)  , struct TYPE * NAME
#define _QUERY_rparam_write(...)

#define _QUERY_wext(...)               _QUERY_wext_ __VA_ARGS__               // unwrap
#define _QUERY_wext_(KIND, ...)        CAT(_QUERY_wext_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_wext_write(TYPE, NAME)  struct TYPE * NAME = (struct TYPE *)data[i++];
#define _QUERY_wext_read(...)

#define _QUERY_rext(...)               _QUERY_rext_ __VA_ARGS__              // unwrap
#define _QUERY_rext_(KIND, ...)        CAT(_QUERY_rext_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rext_read(TYPE, NAME)  struct TYPE * NAME = (struct TYPE *)data[i++];
#define _QUERY_rext_write(...)

#define _QUERY_warg(...)                _QUERY_warg_ __VA_ARGS__               // unwrap
#define _QUERY_warg_(KIND, ...)         CAT(_QUERY_warg_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_warg_write(_TYPE, NAME)  , NAME
#define _QUERY_warg_read(...)

#define _QUERY_rarg(...)                _QUERY_rarg_ __VA_ARGS__               // unwrap
#define _QUERY_rarg_(KIND, ...)         CAT(_QUERY_rarg_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rarg_read(_TYPE, NAME)   , NAME
#define _QUERY_rarg_write(...)

#define QUERY(WORLD, ...)                                                                                                        \
  static alias_ecs_Query * CAT(query, __LINE__) = NULL;                                                                          \
  if(CAT(query, __LINE__) == NULL) {                                                                                             \
    alias_ecs_ComponentHandle _rlist[] = { MAP(_QUERY_rlist, __VA_ARGS__) };                                                     \
    alias_ecs_ComponentHandle _wlist[] = { MAP(_QUERY_wlist, __VA_ARGS__) };                                                     \
    alias_ecs_create_query(WORLD, &(alias_ecs_QueryCreateInfo) {                                                                 \
      .num_write_components = sizeof(_wlist) / sizeof(_wlist[0]),                                                                \
      .write_components = _wlist,                                                                                                \
      .num_read_components = sizeof(_rlist) / sizeof(_rlist[0]),                                                                 \
      .read_components = _rlist                                                                                                  \
    }, &CAT(query, __LINE__));                                                                                                   \
  }                                                                                                                              \
  auto void CAT(query_fn_, __LINE__)                                                                                             \
    ( void * ud                                                                                                                  \
    , alias_ecs_Instance * instance                                                                                              \
    , alias_ecs_EntityHandle entity                                                                                              \
      MAP(_QUERY_wparam, __VA_ARGS__)                                                                                            \
      MAP(_QUERY_rparam, __VA_ARGS__)                                                                                            \
  );                                                                                                                             \
  void CAT(query_fn0_, __LINE__)(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data) {   \
    uint32_t i = 0;                                                                                                              \
    MAP(_QUERY_wext, __VA_ARGS__) \
    MAP(_QUERY_rext, __VA_ARGS__) \
    CAT(query_fn_, __LINE__)(ud, instance, entity MAP(_QUERY_warg, __VA_ARGS__) MAP(_QUERY_rarg, __VA_ARGS__));                  \
  }                                                                                                                              \
  alias_ecs_execute_query(WORLD, CAT(query, __LINE__), (alias_ecs_QueryCB) { CAT(query_fn0_, __LINE__), NULL });                 \
  auto void CAT(query_fn_, __LINE__)                                                                                             \
    ( void * ud                                                                                                                  \
    , alias_ecs_Instance * instance                                                                                              \
    , alias_ecs_EntityHandle entity                                                                                              \
      MAP(_QUERY_wparam, __VA_ARGS__)                                                                                            \
      MAP(_QUERY_rparam, __VA_ARGS__)                                                                                            \
  )

#endif // _UTIL_H_

