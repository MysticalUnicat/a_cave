#ifndef _ENGINE_H_
#define _ENGINE_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#include <alias/ecs.h>
#include <alias/math.h>
#include <alias/color.h>

#include "util.h"

// parameters
#define MAX_INPUT_FRONTEND_SETS 8

// Engine is the only 'singleton'
struct State {
  struct State * prev;
  void * ud;
  void (*begin)(void * ud);
  void (*frame)(void * ud);
  void (*background)(void * ud);
  void (*hud)(void * ud);
  void (*pause)(void * ud);
  void (*unpause)(void * ud);
  void (*end)(void * ud);
};

void Engine_init(uint32_t screen_width, uint32_t screen_height, const char * title, struct State * first_state);

bool Engine_update(void);

// state
void Engine_push_state(struct State * state);

void Engine_pop_state(void);

// parameter access
alias_ecs_Instance * Engine_ecs(void);

alias_R Engine_physics_speed(void);
void Engine_set_physics_speed(alias_R speed);

alias_R Engine_time(void);

// input
enum InputSource {
  Keyboard_Apostrophe,
  Keyboard_Comma,
  Keyboard_Minus,
  Keyboard_Period,
  Keyboard_Slash,
  Keyboard_0,
  Keyboard_1,
  Keyboard_2,
  Keyboard_3,
  Keyboard_4,
  Keyboard_5,
  Keyboard_6,
  Keyboard_7,
  Keyboard_8,
  Keyboard_9,
  Keyboard_Semicolon,
  Keyboard_Equal,
  Keyboard_A,
  Keyboard_B,
  Keyboard_C,
  Keyboard_D,
  Keyboard_E,
  Keyboard_F,
  Keyboard_G,
  Keyboard_H,
  Keyboard_I,
  Keyboard_J,
  Keyboard_K,
  Keyboard_L,
  Keyboard_M,
  Keyboard_N,
  Keyboard_O,
  Keyboard_P,
  Keyboard_Q,
  Keyboard_R,
  Keyboard_S,
  Keyboard_T,
  Keyboard_U,
  Keyboard_V,
  Keyboard_W,
  Keyboard_X,
  Keyboard_Y,
  Keyboard_Z,
  Keyboard_Space,
  Keyboard_Escape,
  Keyboard_Enter,
  Keyboard_Tab,
  Keyboard_Backspace,
  Keyboard_Insert,
  Keyboard_Delete,
  Keyboard_Right,
  Keyboard_Left,
  Keyboard_Down,
  Keyboard_Up,
  Keyboard_PageUp,
  Keyboard_PageDown,
  Keyboard_Home,
  Keyboard_End,
  Keyboard_CapsLock,
  Keyboard_ScrollLock,
  Keyboard_NumLock,
  Keyboard_PrintScreen,
  Keyboard_Pause,
  Keyboard_F1,
  Keyboard_F2,
  Keyboard_F3,
  Keyboard_F4,
  Keyboard_F5,
  Keyboard_F6,
  Keyboard_F7,
  Keyboard_F8,
  Keyboard_F9,
  Keyboard_F10,
  Keyboard_F11,
  Keyboard_F12,
  Keyboard_LeftShift,
  Keyboard_LeftControl,
  Keyboard_Leftalt,
  Keyboard_LeftMeta,
  Keyboard_RightShift,
  Keyboard_RightControl,
  Keyboard_RightAlt,
  Keyboard_RightMeta,
  Keyboard_Menu,
  Keyboard_LeftBracket,
  Keyboard_Backslash,
  Keyboard_RightBracket,
  Keyboard_Grave,
  Keyboard_Pad_0,
  Keyboard_Pad_1,
  Keyboard_Pad_2,
  Keyboard_Pad_3,
  Keyboard_Pad_4,
  Keyboard_Pad_5,
  Keyboard_Pad_6,
  Keyboard_Pad_7,
  Keyboard_Pad_8,
  Keyboard_Pad_9,
  Keyboard_Pad_Period,
  Keyboard_Pad_Divide,
  Keyboard_Pad_Multiply,
  Keyboard_Pad_Minus,
  Keyboard_Pad_Add,
  Keyboard_Pad_Enter,
  Keyboard_Pad_Equal,

  Mouse_Left_Button,
  Mouse_Right_Button,
  Mouse_Position_X,
  Mouse_Position_Y,

  InputSource_COUNT
};

struct InputBackendPair {
  enum InputSource source;
  uint32_t binding;
};

enum InputSignalType {
  InputSignal_Pass,
  InputSignal_Up,
  InputSignal_Down,
  InputSignal_Direction,
  InputSignal_Point,
};

struct InputSignal {
  enum InputSignalType type;
  uint32_t bindings[2];
  union {
    bool boolean;
    alias_pga2d_Direction direction;
    alias_pga2d_Point point;
  };
  union {
    bool up;
    bool down;
  } internal;
};

#define INPUT_SIGNAL_PASS(BINDING) (struct InputSignal) { .type = InputSignal_Pass, .bindings[0] = BINDING }
#define INPUT_SIGNAL_UP(BINDING) (struct InputSignal) { .type = InputSignal_Up, .bindings[0] = BINDING }
#define INPUT_SIGNAL_DOWN(BINDING) (struct InputSignal) { .type = InputSignal_Down, .bindings[0] = BINDING }
#define INPUT_SIGNAL_DIRECTION(BINDING_X, BINDING_Y) (struct InputSignal) { .type = InputSignal_Direction, .bindings[0] = BINDING_X, .bindings[1] = BINDING_Y }
#define INPUT_SIGNAL_POINT(BINDING_X, BINDING_Y) (struct InputSignal) { .type = InputSignal_Point, .bindings[0] = BINDING_X, .bindings[1] = BINDING_Y }

void Engine_set_player_input_backend(uint32_t player_index, uint32_t pair_count, const struct InputBackendPair * pairs);

uint32_t Engine_add_input_frontend(uint32_t player_index, uint32_t signal_count, struct InputSignal * signals);
void Engine_remove_input_frontend(uint32_t player_index, uint32_t index);

// event
DECLARE_COMPONENT(Event, {
  uint32_t id;
})

#define SPAWN_EVENT(...) SPAWN(( Event, .id = event_id() ), ## __VA_ARGS__)

#define QUERY_EVENT(...) \
  static uint32_t CAT(_leid, __LINE__) = 0; \
  uint32_t CAT(_neid, __LINE__) = CAT(_leid, __LINE__); \
  _QUERY( \
    if(_event->id <= CAT(_leid, __LINE__)) return; \
    if(_event->id > CAT(_neid, __LINE__)) CAT(_neid, __LINE__) = _event->id; \
  , \
    CAT(_leid, __LINE__) = CAT(_neid, __LINE__); \
  , ( read, Event, _event ), ## __VA_ARGS__)

uint32_t Engine_next_event_id(void);

#define ENGINE_COMPONENT(BUNDLE, IDENT)                                                                      \
  static inline alias_ecs_ComponentHandle alias_##IDENT##_component(void) {                                  \
    return BUNDLE()->IDENT##_component;                                                                      \
  }                                                                                                          \
  static inline const struct alias_##IDENT * alias_##IDENT##_read(Entity entity) {                           \
    const struct alias_##IDENT * ptr;                                                                        \
    alias_ecs_read_entity_component(Engine_ecs(), entity, BUNDLE()->IDENT##_component, (const void **)&ptr); \
    return ptr;                                                                                              \
  }                                                                                                          \
  static inline struct alias_##IDENT * alias_##IDENT##_write(Entity entity) {                                \
    struct alias_##IDENT * ptr;                                                                              \
    alias_ecs_write_entity_component(Engine_ecs(), entity, BUNDLE()->IDENT##_component, (void **)&ptr);      \
    return ptr;                                                                                              \
  }

// transform
#include <alias/transform.h>

alias_TransformBundle * Engine_transform_bundle(void);

ENGINE_COMPONENT(Engine_transform_bundle, Translation2D)
ENGINE_COMPONENT(Engine_transform_bundle, Rotation2D)
ENGINE_COMPONENT(Engine_transform_bundle, Transform2D)
ENGINE_COMPONENT(Engine_transform_bundle, LocalToWorld2D)
ENGINE_COMPONENT(Engine_transform_bundle, Parent2D)

// physics
#include <alias/physics.h>

alias_Physics2DBundle * Engine_physics_2d_bundle(void);

ENGINE_COMPONENT(Engine_physics_2d_bundle, Physics2DMotion)
ENGINE_COMPONENT(Engine_physics_2d_bundle, Physics2DBodyMotion)
ENGINE_COMPONENT(Engine_physics_2d_bundle, Physics2DMass)
ENGINE_COMPONENT(Engine_physics_2d_bundle, Physics2DDampen)

// render
struct LoadedResource;

struct Image {
  const char * path;
  uint32_t resource_id;
  struct LoadedResource * resource;
};

DECLARE_COMPONENT(Camera, {
  alias_pga2d_Point viewport_min;
  alias_pga2d_Point viewport_max;
  alias_R zoom;
})

#define Camera_DEFAULT (struct Camera) { .viewport_max = { alias_R_ONE, alias_R_ONE }, .zoom = alias_R_ONE }

DECLARE_COMPONENT(DrawRectangle, {
  float width;
  float height;
  alias_Color color;
})

DECLARE_COMPONENT(DrawCircle, {
  alias_R radius;
  alias_Color color;
})

DECLARE_COMPONENT(DrawText, {
  const char * text;
  alias_R size;
  alias_Color color;
})

DECLARE_COMPONENT(Sprite, {
  struct Image * image;
  alias_R s0, t0, s1, t1;
  alias_Color color;
})

// ui
void Engine_ui_align_fractions(float x, float y);

static inline void Engine_ui_top_left(void) { Engine_ui_align_fractions(0, 0); }
static inline void Engine_ui_top(void) { Engine_ui_align_fractions(0.5, 0); }
static inline void Engine_ui_top_right(void) { Engine_ui_align_fractions(1, 0); }
static inline void Engine_ui_left(void) { Engine_ui_align_fractions(0, 0.5); }
static inline void Engine_ui_center(void) { Engine_ui_align_fractions(0.5, 0.5); }
static inline void Engine_ui_right(void) { Engine_ui_align_fractions(1, 0.5); }
static inline void Engine_ui_bottom_left(void) { Engine_ui_align_fractions(0, 1); }
static inline void Engine_ui_bottom(void) { Engine_ui_align_fractions(0.5, 1); }
static inline void Engine_ui_bottom_right(void) { Engine_ui_align_fractions(1, 1); }

void Engine_ui_image(struct Image *);

void Engine_ui_vertical(void);
void Engine_ui_horizontal(void);
void Engine_ui_stack(void);
void Engine_ui_end(void);

void Engine_ui_font_size(alias_R size);
void Engine_ui_font_color(alias_Color color);
void Engine_ui_text(const char * format, ...);

#endif
