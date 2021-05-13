#ifndef _UTIL_H_
#define _UTIL_H_

#include <alias/ecs.h>
#include <raylib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "pp.h"

#define Entity alias_ecs_EntityHandle

extern alias_ecs_Instance * g_world;

#define LAZY_GLOBAL(TYPE, IDENT, ...) \
  TYPE IDENT (void) {                 \
    static TYPE inner;                \
    static int init = 0;              \
    if(init == 0) {                   \
      printf("making "#IDENT" ...\n");\
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
      printf("making "#IDENT" ...\n");    \
      __VA_ARGS__                         \
      __ptr = &inner;                     \
    }                                     \
    return __ptr;                         \
  }

#define ECS(F, ...) assert(ALIAS_ECS_SUCCESS == alias_ecs_##F(__VA_ARGS__))

#define DEFINE_FONT(IDENT, PATH) LAZY_GLOBAL_PTR(Font, IDENT, inner = LoadFont(PATH);)

#define DECLARE_COMPONENT(IDENT, ...)                \
  struct IDENT __VA_ARGS__;                          \
  alias_ecs_ComponentHandle IDENT##_component(void); \
  const struct IDENT * IDENT##_read(Entity entity);  \
  struct IDENT * IDENT##_write(Entity entity);

#define DEFINE_COMPONENT(IDENT)                                                                         \
  LAZY_GLOBAL(                                                                                                 \
    alias_ecs_ComponentHandle,                                                                                 \
    IDENT##_component,                                                                                         \
    ECS(register_component, g_world, &(alias_ecs_ComponentCreateInfo) { .size = sizeof(struct IDENT) }, &inner); \
  )                                                                                                            \
  const struct IDENT * IDENT##_read(Entity entity) {                                                           \
    const struct IDENT * ptr;                                                                                  \
    ECS(read_entity_component, g_world, entity, IDENT##_component(), (const void **)&ptr);                       \
    return ptr;                                                                                                \
  }                                                                                                            \
  struct IDENT * IDENT##_write(Entity entity) {                                                                \
    struct IDENT * ptr;                                                                                        \
    ECS(write_entity_component, g_world, entity, IDENT##_component(), (void **)&ptr);                            \
    return ptr;                                                                                                \
  }

#define DECLARE_TAG_COMPONENT(IDENT) \
  alias_ecs_ComponentHandle IDENT##_component(void);

#define DEFINE_TAG_COMPONENT(IDENT)                                                  \
  LAZY_GLOBAL(                                                                              \
    alias_ecs_ComponentHandle,                                                              \
    IDENT##_component,                                                                      \
    ECS(register_component, g_world, &(alias_ecs_ComponentCreateInfo) { .size = 0 }, &inner); \
  )

#define SPAWN_COMPONENT(...) SPAWN_COMPONENT_ __VA_ARGS__
#define SPAWN_COMPONENT_(TYPE, ...) { .component = TYPE##_component(), .stride = sizeof(struct TYPE), .data = (void *)&(struct TYPE) { __VA_ARGS__ } },
#define SPAWN(...) ({                                               \
  alias_ecs_EntityHandle _entity;                                   \
  alias_ecs_EntitySpawnComponent _components[] = {                  \
    MAP(SPAWN_COMPONENT, __VA_ARGS__)                               \
  };                                                                \
  ECS(spawn, g_world, &(alias_ecs_EntitySpawnInfo) {                  \
    .layer = ALIAS_ECS_INVALID_LAYER,                               \
    .count = 1,                                                     \
    .num_components = sizeof(_components) / sizeof(_components[0]), \
    .components = _components                                       \
  }, &_entity);                                                     \
  _entity;                                                          \
})

#define _QUERY_wlist(...)               _QUERY_wlist_ __VA_ARGS__               // unwrap
#define _QUERY_wlist_(KIND, ...)        CAT(_QUERY_wlist_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_wlist_read(...)
#define _QUERY_wlist_write(TYPE, _NAME) TYPE##_component(),

#define _QUERY_rlist(...)               _QUERY_rlist_ __VA_ARGS__               // unwrap
#define _QUERY_rlist_(KIND, ...)        CAT(_QUERY_rlist_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rlist_read(TYPE, _NAME)  TYPE##_component(),
#define _QUERY_rlist_write(...)

#define _QUERY_wparam(...)              _QUERY_wparam_ __VA_ARGS__              // unwrap
#define _QUERY_wparam_(KIND, ...)       CAT(_QUERY_wparam_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_wparam_read(...)
#define _QUERY_wparam_write(TYPE, NAME) , struct TYPE * NAME

#define _QUERY_rparam(...)              _QUERY_rparam_ __VA_ARGS__              // unwrap
#define _QUERY_rparam_(KIND, ...)       CAT(_QUERY_rparam_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rparam_read(TYPE, NAME)  , struct TYPE * NAME
#define _QUERY_rparam_write(...)

#define _QUERY_wext(...)               _QUERY_wext_ __VA_ARGS__               // unwrap
#define _QUERY_wext_(KIND, ...)        CAT(_QUERY_wext_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_wext_read(...)
#define _QUERY_wext_write(TYPE, NAME)  struct TYPE * NAME = (struct TYPE *)data[i++];

#define _QUERY_rext(...)               _QUERY_rext_ __VA_ARGS__              // unwrap
#define _QUERY_rext_(KIND, ...)        CAT(_QUERY_rext_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rext_read(TYPE, NAME)  struct TYPE * NAME = (struct TYPE *)data[i++];
#define _QUERY_rext_write(...)

#define _QUERY_warg(...)                _QUERY_warg_ __VA_ARGS__               // unwrap
#define _QUERY_warg_(KIND, ...)         CAT(_QUERY_warg_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_warg_read(...)
#define _QUERY_warg_write(_TYPE, NAME)  , NAME

#define _QUERY_rarg(...)                _QUERY_rarg_ __VA_ARGS__               // unwrap
#define _QUERY_rarg_(KIND, ...)         CAT(_QUERY_rarg_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rarg_read(_TYPE, NAME)   , NAME
#define _QUERY_rarg_write(...)

#define _QUERY(EACH_INJECT, POST_INJECT, ...) \
  static alias_ecs_Query * CAT(query, __LINE__) = NULL; \
  if(CAT(query, __LINE__) == NULL) { \
    alias_ecs_ComponentHandle _rlist[] = { MAP(_QUERY_rlist, __VA_ARGS__) }; \
    alias_ecs_ComponentHandle _wlist[] = { MAP(_QUERY_wlist, __VA_ARGS__) }; \
    alias_ecs_create_query(g_world, &(alias_ecs_QueryCreateInfo) { \
      .num_write_components = sizeof(_wlist) / sizeof(_wlist[0]), \
      .write_components = _wlist, \
      .num_read_components = sizeof(_rlist) / sizeof(_rlist[0]), \
      .read_components = _rlist \
    }, &CAT(query, __LINE__)); \
  } \
  auto void CAT(query_fn_, __LINE__) \
    ( void * ud \
    , alias_ecs_Instance * instance \
    , alias_ecs_EntityHandle entity \
      MAP(_QUERY_wparam, __VA_ARGS__) \
      MAP(_QUERY_rparam, __VA_ARGS__) \
  ); \
  void CAT(query_fn0_, __LINE__)(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data) { \
    uint32_t i = 0; \
    MAP(_QUERY_wext, __VA_ARGS__) \
    MAP(_QUERY_rext, __VA_ARGS__) \
    EACH_INJECT \
    CAT(query_fn_, __LINE__)(ud, instance, entity MAP(_QUERY_warg, __VA_ARGS__) MAP(_QUERY_rarg, __VA_ARGS__)); \
  } \
  alias_ecs_execute_query(g_world, CAT(query, __LINE__), (alias_ecs_QueryCB) { CAT(query_fn0_, __LINE__), NULL }); \
  POST_INJECT \
  auto void CAT(query_fn_, __LINE__) \
    ( void * ud \
    , alias_ecs_Instance * instance \
    , alias_ecs_EntityHandle entity \
      MAP(_QUERY_wparam, __VA_ARGS__) \
      MAP(_QUERY_rparam, __VA_ARGS__) \
  )

#define QUERY(...) _QUERY(,, __VA_ARGS__)

struct Cmd {
  uint32_t size;
  enum {
    cmd_add_component,
    cmd_remove_component,
    cmd_despawn
  } tag;
  alias_ecs_EntityHandle entity;
  alias_ecs_ComponentHandle component;
};

struct CmdBuf {
  void * start;
  void * pointer;
  void * end;
};

void CmdBuf_add_component(struct CmdBuf * cbuf, alias_ecs_EntityHandle entity, alias_ecs_ComponentHandle component, void * data, size_t data_size);
void CmdBuf_remove_component(struct CmdBuf * cbuf, alias_ecs_EntityHandle entity, alias_ecs_ComponentHandle component);
void CmdBuf_despawn(struct CmdBuf * cbuf, alias_ecs_EntityHandle entity);
void CmdBuf_begin_recording(struct CmdBuf * cbuf);
void CmdBuf_end_recording(struct CmdBuf * cbuf);
void CmdBuf_execute(struct CmdBuf * cbuf, alias_ecs_Instance * instance);

#endif // _UTIL_H_

