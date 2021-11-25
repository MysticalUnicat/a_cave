#ifndef _UTIL_H_
#define _UTIL_H_

#include <alias/ecs.h>
#include <alias/cpp.h>
//#include <raylib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "pp.h"

#define Entity alias_ecs_EntityHandle

extern alias_ecs_Instance * Engine_ecs(void);

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

#define DEFINE_COMPONENT(IDENT, ...)                                                                 \
  LAZY_GLOBAL(                                                                                       \
    alias_ecs_ComponentHandle,                                                                       \
    IDENT##_component,                                                                               \
    ECS(register_component, Engine_ecs()                                                             \
      , &(alias_ecs_ComponentCreateInfo) { .size = sizeof(struct IDENT), ## __VA_ARGS__ }, &inner);  \
  )                                                                                                  \
  const struct IDENT * IDENT##_read(Entity entity) {                                                 \
    const struct IDENT * ptr;                                                                        \
    alias_ecs_read_entity_component(Engine_ecs(), entity, IDENT##_component(), (const void **)&ptr); \
    return ptr;                                                                                      \
  }                                                                                                  \
  struct IDENT * IDENT##_write(Entity entity) {                                                      \
    struct IDENT * ptr;                                                                              \
    alias_ecs_write_entity_component(Engine_ecs(), entity, IDENT##_component(), (void **)&ptr);      \
    return ptr;                                                                                      \
  }

#define DECLARE_TAG_COMPONENT(IDENT) \
  alias_ecs_ComponentHandle IDENT##_component(void);

#define DEFINE_TAG_COMPONENT(IDENT)                                                                \
  LAZY_GLOBAL(                                                                                     \
    alias_ecs_ComponentHandle,                                                                     \
    IDENT##_component,                                                                             \
    ECS(register_component, Engine_ecs(), &(alias_ecs_ComponentCreateInfo) { .size = 0 }, &inner); \
  )

#define COMPONENT_impl(IDENT, ITYPE, DREF, ...)                                                      \
  LAZY_GLOBAL(                                                                                       \
    alias_ecs_ComponentHandle,                                                                       \
    IDENT##_component,                                                                               \
    ECS(register_component, Engine_ecs()                                                             \
    , &(alias_ecs_ComponentCreateInfo) { .size = sizeof(ITYPE), ## __VA_ARGS__ }, &inner);           \
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

#define SPAWN_COMPONENT(...) SPAWN_COMPONENT_ __VA_ARGS__
#define SPAWN_COMPONENT_(TYPE, ...) { .component = TYPE##_component(), .stride = sizeof(struct TYPE), .data = (void *)&(struct TYPE) { __VA_ARGS__ } },

#define SPAWN_LAYER(LAYER, ...) ({                                  \
  alias_ecs_EntityHandle _entity;                                   \
  alias_ecs_EntitySpawnComponent _components[] = {                  \
    ALIAS_CPP_EVAL(ALIAS_CPP_MAP(SPAWN_COMPONENT, __VA_ARGS__))     \
  };                                                                \
  ECS(spawn, Engine_ecs(), &(alias_ecs_EntitySpawnInfo) {           \
    .layer = LAYER,                                                 \
    .count = 1,                                                     \
    .num_components = sizeof(_components) / sizeof(_components[0]), \
    .components = _components                                       \
  }, &_entity);                                                     \
  _entity;                                                          \
})

#define SPAWN(...) SPAWN_LAYER(ALIAS_ECS_INVALID_LAYER, ## __VA_ARGS__)

#if 0
#define _QUERY_wlist(...)               _QUERY_wlist_ __VA_ARGS__               // unwrap
#define _QUERY_wlist_(KIND, ...)        CAT(_QUERY_wlist_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_wlist_read(...)
#define _QUERY_wlist_write(TYPE, _NAME) TYPE##_component(),
#define _QUERY_wlist_filter(...)

#define _QUERY_rlist(...)               _QUERY_rlist_ __VA_ARGS__               // unwrap
#define _QUERY_rlist_(KIND, ...)        CAT(_QUERY_rlist_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rlist_read(TYPE, _NAME)  TYPE##_component(),
#define _QUERY_rlist_write(...)
#define _QUERY_rlist_filter(...)

#define _QUERY_FILTER_MAP_optional ALIAS_ECS_FILTER_OPTIONAL
#define _QUERY_FILTER_MAP_exclude ALIAS_ECS_FILTER_EXCLUDE
#define _QUERY_FILTER_MAP_modified ALIAS_ECS_FILTER_MODIFIED

#define _QUERY_flist(...)               _QUERY_flist_ __VA_ARGS__               // unwrap
#define _QUERY_flist_(KIND, ...)        CAT(_QUERY_flist_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_flist_read(...)
#define _QUERY_flist_write(...)
#define _QUERY_flist_filter(E, TYPE)    { .component = TYPE##_component(), .filter = CAT(_QUERY_FILTER_MAP_, E) },

#define _QUERY_wparam(...)              _QUERY_wparam_ __VA_ARGS__              // unwrap
#define _QUERY_wparam_(KIND, ...)       CAT(_QUERY_wparam_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_wparam_read(...)
#define _QUERY_wparam_write(TYPE, NAME) , struct TYPE * NAME
#define _QUERY_wparam_filter(...)

#define _QUERY_rparam(...)              _QUERY_rparam_ __VA_ARGS__              // unwrap
#define _QUERY_rparam_(KIND, ...)       CAT(_QUERY_rparam_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rparam_read(TYPE, NAME)  , struct TYPE * NAME
#define _QUERY_rparam_write(...)
#define _QUERY_rparam_filter(...)

#define _QUERY_wext(...)               _QUERY_wext_ __VA_ARGS__               // unwrap
#define _QUERY_wext_(KIND, ...)        CAT(_QUERY_wext_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_wext_read(...)
#define _QUERY_wext_write(TYPE, NAME)  struct TYPE * NAME = (struct TYPE *)data[i++];
#define _QUERY_wext_filter(...)

#define _QUERY_rext(...)               _QUERY_rext_ __VA_ARGS__              // unwrap
#define _QUERY_rext_(KIND, ...)        CAT(_QUERY_rext_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rext_read(TYPE, NAME)  struct TYPE * NAME = (struct TYPE *)data[i++];
#define _QUERY_rext_write(...)
#define _QUERY_rext_filter(...)

#define _QUERY_warg(...)                _QUERY_warg_ __VA_ARGS__               // unwrap
#define _QUERY_warg_(KIND, ...)         CAT(_QUERY_warg_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_warg_read(...)
#define _QUERY_warg_write(_TYPE, NAME)  , NAME
#define _QUERY_warg_filter(...)

#define _QUERY_rarg(...)                _QUERY_rarg_ __VA_ARGS__               // unwrap
#define _QUERY_rarg_(KIND, ...)         CAT(_QUERY_rarg_, KIND) (__VA_ARGS__) // add kind
#define _QUERY_rarg_read(_TYPE, NAME)   , NAME
#define _QUERY_rarg_write(...)
#define _QUERY_rarg_filter(...)

#define _QUERY(NAME, PRE, EACH, POST, ...) \
  void CAT(NAME, _do) \
    ( void * ud \
    , alias_ecs_Instance * instance \
    , alias_ecs_EntityHandle entity \
      MAP(_QUERY_wparam, __VA_ARGS__) \
      MAP(_QUERY_rparam, __VA_ARGS__) \
  ); \
  static void CAT(NAME, _do_wrap)(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data) { \
    uint32_t i = 0; \
    MAP(_QUERY_wext, __VA_ARGS__) \
    MAP(_QUERY_rext, __VA_ARGS__) \
    EACH \
    CAT(NAME, _do)(ud, instance, entity MAP(_QUERY_warg, __VA_ARGS__) MAP(_QUERY_rarg, __VA_ARGS__)); \
  } \
  void NAME(void) { \
    static alias_ecs_Query * query = NULL; \
    if(query == NULL) { \
      alias_ecs_ComponentHandle _rlist[] = { MAP(_QUERY_rlist, __VA_ARGS__) }; \
      alias_ecs_ComponentHandle _wlist[] = { MAP(_QUERY_wlist, __VA_ARGS__) }; \
      alias_ecs_QueryFilterCreateInfo _flist[] = { MAP(_QUERY_flist, __VA_ARGS__) }; \
      alias_ecs_create_query(Engine_ecs(), &(alias_ecs_QueryCreateInfo) { \
        .num_write_components = sizeof(_wlist) / sizeof(_wlist[0]), \
        .write_components = _wlist, \
        .num_read_components = sizeof(_rlist) / sizeof(_rlist[0]), \
        .read_components = _rlist, \
        .num_filters = sizeof(_flist) / sizeof(_flist[0]), \
        .filters = _flist \
      }, &query); \
    } \
    PRE \
    alias_ecs_execute_query(Engine_ecs(), query, (alias_ecs_QueryCB) { CAT(NAME, _do_wrap), NULL }); \
    POST \
  } \
  void CAT(NAME, _do) \
    ( void * ud \
    , alias_ecs_Instance * instance \
    , alias_ecs_EntityHandle entity \
      MAP(_QUERY_wparam, __VA_ARGS__) \
      MAP(_QUERY_rparam, __VA_ARGS__) \
  )

#define QUERY(NAME, ...) _QUERY(NAME, , , , __VA_ARGS__)
#endif

#define ALIAS_CPP_EQ__QUERYstate_state(...) ALIAS_CPP_PROBE
#define ALIAS_CPP_EQ__QUERYpre_pre(...)     ALIAS_CPP_PROBE
#define ALIAS_CPP_EQ__QUERYread_read(...)   ALIAS_CPP_PROBE
#define ALIAS_CPP_EQ__QUERYwrite_write(...) ALIAS_CPP_PROBE
#define ALIAS_CPP_EQ__QUERYoptional_optional(...) ALIAS_CPP_PROBE
#define ALIAS_CPP_EQ__QUERYexclude_exclude(...) ALIAS_CPP_PROBE
#define ALIAS_CPP_EQ__QUERYmodified_modified(...) ALIAS_CPP_PROBE
#define ALIAS_CPP_EQ__QUERYaction_action(...)   ALIAS_CPP_PROBE
#define ALIAS_CPP_EQ__QUERYpost_post(...)   ALIAS_CPP_PROBE

#define QUERY_is_state(X) ALIAS_CPP_EQ(QUERY, state, X)
#define QUERY_is_pre(X) ALIAS_CPP_EQ(QUERY, pre, X)
#define QUERY_is_read(X) ALIAS_CPP_EQ(QUERY, read, X)
#define QUERY_is_write(X) ALIAS_CPP_EQ(QUERY, write, X)
#define QUERY_is_filter(X) ALIAS_CPP_OR(ALIAS_CPP_OR(ALIAS_CPP_EQ(QUERY, optional, X), ALIAS_CPP_EQ(QUERY, exclude, X)), ALIAS_CPP_EQ(QUERY, modified, X))
#define QUERY_is_action(X) ALIAS_CPP_EQ(QUERY, action, X)
#define QUERY_is_post(X) ALIAS_CPP_EQ(QUERY, post, X)

#define QUERY_emit(X) ALIAS_CPP_CAT(QUERY_emit_, X)
#define QUERY_emit_state(TYPE, NAME, ...) TYPE NAME;
#define QUERY_emit_write(TYPE, NAME) struct TYPE * NAME = (struct TYPE *)data[__i++];
#define QUERY_emit_read(TYPE, NAME) const struct TYPE * NAME = (const struct TYPE *)data[__i++];
#define QUERY_emit_pre(...) __VA_ARGS__
#define QUERY_emit_action(...) __VA_ARGS__
#define QUERY_emit_post(...) __VA_ARGS__

#define QUERY_emit_create(X) ALIAS_CPP_CAT(QUERY_emit_create_, X)
#define QUERY_emit_create_read(TYPE, NAME) TYPE##_component(),
#define QUERY_emit_create_write(TYPE, NAME) TYPE##_component(),
#define QUERY_emit_create_optional(TYPE) { .component = TYPE##_component(), .filter = ALIAS_ECS_FILTER_OPTIONAL },
#define QUERY_emit_create_exclude(TYPE) { .component = TYPE##_component(), .filter = ALIAS_ECS_FILTER_EXCLUDE },
#define QUERY_emit_create_modified(TYPE) { .component = TYPE##_component(), .filter = ALIAS_ECS_FILTER_MODIFIED },

#define QUERY(NAME, ...) ALIAS_CPP_EVAL(QUERY_impl(NAME, __VA_ARGS__))
#define QUERY_impl(NAME, ...) \
  struct ALIAS_CPP_CAT(NAME, _state) { \
    alias_ecs_Query * query; \
    ALIAS_CPP_FILTER_MAP(QUERY_is_state, QUERY_emit, __VA_ARGS__) \
  }; \
  static void ALIAS_CPP_CAT(NAME, _do)(void * ud, alias_ecs_Instance * instance, alias_ecs_EntityHandle entity, void ** data) { \
    uint32_t __i = 0; \
    struct ALIAS_CPP_CAT(NAME, _state) * state = (struct ALIAS_CPP_CAT(NAME, _state) *)ud; \
    ALIAS_CPP_FILTER_MAP(QUERY_is_write, QUERY_emit, __VA_ARGS__) \
    ALIAS_CPP_FILTER_MAP(QUERY_is_read, QUERY_emit, __VA_ARGS__) \
    ALIAS_CPP_FILTER_MAP(QUERY_is_action, QUERY_emit, __VA_ARGS__) \
  } \
  void NAME(void) { \
    static struct ALIAS_CPP_CAT(NAME, _state) _state = { 0 }; \
    static struct ALIAS_CPP_CAT(NAME, _state) * state = &_state; \
    if(state->query == NULL) { \
      alias_ecs_ComponentHandle _rlist[] = { ALIAS_CPP_FILTER_MAP(QUERY_is_read, QUERY_emit_create, __VA_ARGS__) }; \
      alias_ecs_ComponentHandle _wlist[] = { ALIAS_CPP_FILTER_MAP(QUERY_is_write, QUERY_emit_create, __VA_ARGS__) }; \
      alias_ecs_QueryFilterCreateInfo _flist[] = { ALIAS_CPP_FILTER_MAP(QUERY_is_filter, QUERY_emit_create, __VA_ARGS__) }; \
      alias_ecs_create_query(Engine_ecs(), &(alias_ecs_QueryCreateInfo) { \
        .num_write_components = sizeof(_wlist) / sizeof(_wlist[0]), \
        .write_components = _wlist, \
        .num_read_components = sizeof(_rlist) / sizeof(_rlist[0]), \
        .read_components = _rlist, \
        .num_filters = sizeof(_flist) / sizeof(_flist[0]), \
        .filters = _flist \
      }, &state->query); \
    } \
    ALIAS_CPP_FILTER_MAP(QUERY_is_pre, QUERY_emit, __VA_ARGS__) \
    alias_ecs_execute_query(Engine_ecs(), state->query, (alias_ecs_QueryCB) { ALIAS_CPP_CAT(NAME, _do), state }); \
    ALIAS_CPP_FILTER_MAP(QUERY_is_post, QUERY_emit, __VA_ARGS__) \
  }

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
