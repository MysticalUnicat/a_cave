#ifndef _ENGINE_H_
#define _ENGINE_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#include <alias/ecs.h>
#include <alias/math.h>

#include "util.h"

// Engine is the only 'singleton'
struct State {
  struct State * prev;
  void * ud;
  void (*begin)(void * ud);
  void (*frame)(void * ud);
  void (*background)(void * ud);
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
  InputSignal_Up,
  InputSignal_Vector2D
};

struct InputSignalUp {
  enum InputSignalType type;
  bool value;
  uint32_t binding;
  bool _internal_1;
};

struct InputSignalVector2D {
  enum InputSignalType type;
  alias_Vector2D value;
  uint32_t binding_x;
  uint32_t binding_y;
};

union InputSignal {
  enum InputSignalType type;
  struct InputSignalUp up;
  struct InputSignalVector2D vector2d;
};

void Engine_set_player_input_backend(uint32_t player_index, uint32_t pair_count, const struct InputBackendPair * pairs);
void Engine_set_player_input_frontend(uint32_t player_index, uint32_t signal_count, union InputSignal * * signals);

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

// transform
#include <alias/transform.h>

alias_TransformBundle * Engine_transform_bundle(void);

static inline alias_ecs_ComponentHandle alias_Translation2D_component(void) {
  return Engine_transform_bundle()->Translation2D_component;
}

static inline alias_ecs_ComponentHandle alias_Rotation2D_component(void) {
  return Engine_transform_bundle()->Rotation2D_component;
}

static inline alias_ecs_ComponentHandle alias_LocalToWorld2D_component(void) {
  return Engine_transform_bundle()->LocalToWorld2D_component;
}

static inline alias_ecs_ComponentHandle alias_Parent2D_component(void) {
  return Engine_transform_bundle()->Parent2D_component;
}

// physics
#include <alias/physics.h>

alias_Physics2DBundle * Engine_physics_2d_bundle(void);

static inline alias_ecs_ComponentHandle alias_Physics2DLinearMotion_component(void) {
  return Engine_physics_2d_bundle()->Physics2DLinearMotion_component;
}

static inline alias_ecs_ComponentHandle alias_Physics2DLinearMass_component(void) {
  return Engine_physics_2d_bundle()->Physics2DLinearMass_component;
}

// render
struct Image {
  const char * path;
  void * loaded;
};

struct Color {
  union {
    struct {
      uint8_t r;
      uint8_t g;
      uint8_t b;
      uint8_t a;
    };
    uint32_t cpu_endian;
  };
};

extern const struct Color Color_WHITE;
extern const struct Color Color_GRAY;
extern const struct Color Color_BLACK;
extern const struct Color Color_TRANSPARENT_BLACK;

static inline uint32_t Color_rgba_u32(struct Color c) {
  return c.cpu_endian;
}

DECLARE_COMPONENT(DrawRectangle, {
  float width;
  float height;
  struct Color color;
})

DECLARE_COMPONENT(DrawCircle, {
  float radius;
  struct Color color;
})

DECLARE_COMPONENT(DrawText, {
  const char * text;
  float size;
  struct Color color;
})

// hud
DECLARE_COMPONENT(HudTransform, {
  alias_Vector2D offset;
  alias_R width;
  alias_R height;
})

DECLARE_COMPONENT(HudAnchor, {
  alias_Vector2D min;
  alias_Vector2D max;
})

DECLARE_COMPONENT(HudParent, {
  Entity parent;
})

DECLARE_COMPONENT(HudText, {
  const char * text;
  float size;
  struct Color color;
})

#endif
