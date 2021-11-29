#include "engine.h"

#include "backend.h"

#include <alias/ui.h>
#include <alias/data_structure/inline_list.h>

#include <uchar.h>

#define Image raylib_Image
#define Color raylib_Color
#include <raylib.h>
#include <rlgl.h>
#undef Image
#undef Color

const struct alias_Color alias_Color_RAYWHITE = (struct alias_Color) { 0.96, 0.96, 0.96, 0.96 };

static inline raylib_Color alias_Color_to_raylib_Color(struct alias_Color c) {
  return (raylib_Color) { c.r * 255.0, c.g * 255.0, c.b * 255.0, c.a * 255.0 };
}

void Backend_init_window(uint32_t width, uint32_t height, const char * title) {
  InitWindow(width, height, title);
  SetExitKey('Q');
}

void Backend_set_target_fps(uint32_t fps) {
  SetTargetFPS(fps);
}

bool Backend_should_exit(void) {
  return WindowShouldClose();
}

static const int _input_source_to_raylib[] = {
  [Keyboard_Apostrophe] =   KEY_APOSTROPHE,
  [Keyboard_Comma] =        KEY_COMMA,
  [Keyboard_Minus] =        KEY_MINUS,
  [Keyboard_Period] =       KEY_PERIOD,
  [Keyboard_Slash] =        KEY_SLASH,
  [Keyboard_0] =            KEY_ZERO,
  [Keyboard_1] =            KEY_ONE,
  [Keyboard_2] =            KEY_TWO,
  [Keyboard_3] =            KEY_THREE,
  [Keyboard_4] =            KEY_FOUR,
  [Keyboard_5] =            KEY_FIVE,
  [Keyboard_6] =            KEY_SIX,
  [Keyboard_7] =            KEY_SEVEN,
  [Keyboard_8] =            KEY_EIGHT,
  [Keyboard_9] =            KEY_NINE,
  [Keyboard_Semicolon] =    KEY_SEMICOLON,
  [Keyboard_Equal] =        KEY_EQUAL,
  [Keyboard_A] =            KEY_A,
  [Keyboard_B] =            KEY_B,
  [Keyboard_C] =            KEY_C,
  [Keyboard_D] =            KEY_D,
  [Keyboard_E] =            KEY_E,
  [Keyboard_F] =            KEY_F,
  [Keyboard_G] =            KEY_G,
  [Keyboard_H] =            KEY_H,
  [Keyboard_I] =            KEY_I,
  [Keyboard_J] =            KEY_J,
  [Keyboard_K] =            KEY_K,
  [Keyboard_L] =            KEY_L,
  [Keyboard_M] =            KEY_M,
  [Keyboard_N] =            KEY_N,
  [Keyboard_O] =            KEY_O,
  [Keyboard_P] =            KEY_P,
  [Keyboard_Q] =            KEY_Q,
  [Keyboard_R] =            KEY_R,
  [Keyboard_S] =            KEY_S,
  [Keyboard_T] =            KEY_T,
  [Keyboard_U] =            KEY_U,
  [Keyboard_V] =            KEY_V,
  [Keyboard_W] =            KEY_X,
  [Keyboard_X] =            KEY_W,
  [Keyboard_Y] =            KEY_Y,
  [Keyboard_Z] =            KEY_Z,
  [Keyboard_Space] =        KEY_SPACE,
  [Keyboard_Escape] =       KEY_ESCAPE,
  [Keyboard_Enter] =        KEY_ENTER,
  [Keyboard_Tab] =          KEY_TAB,
  [Keyboard_Backspace] =    KEY_BACKSPACE,
  [Keyboard_Insert] =       KEY_INSERT,
  [Keyboard_Delete] =       KEY_DELETE,
  [Keyboard_Right] =        KEY_RIGHT,
  [Keyboard_Left] =         KEY_LEFT,
  [Keyboard_Down] =         KEY_DOWN,
  [Keyboard_Up] =           KEY_UP,
  [Keyboard_PageUp] =       KEY_PAGE_UP,
  [Keyboard_PageDown] =     KEY_PAGE_DOWN,
  [Keyboard_Home] =         KEY_HOME,
  [Keyboard_End] =          KEY_END,
  [Keyboard_CapsLock] =     KEY_CAPS_LOCK,
  [Keyboard_ScrollLock] =   KEY_SCROLL_LOCK,
  [Keyboard_NumLock] =      KEY_NUM_LOCK,
  [Keyboard_PrintScreen] =  KEY_PRINT_SCREEN,
  [Keyboard_Pause] =        KEY_PAUSE,
  [Keyboard_F1] =           KEY_F1,
  [Keyboard_F2] =           KEY_F2,
  [Keyboard_F3] =           KEY_F3,
  [Keyboard_F4] =           KEY_F4,
  [Keyboard_F5] =           KEY_F5,
  [Keyboard_F6] =           KEY_F6,
  [Keyboard_F7] =           KEY_F7,
  [Keyboard_F8] =           KEY_F8,
  [Keyboard_F9] =           KEY_F9,
  [Keyboard_F10] =          KEY_F10,
  [Keyboard_F11] =          KEY_F11,
  [Keyboard_F12] =          KEY_F12,
  [Keyboard_LeftShift] =    KEY_LEFT_SHIFT,
  [Keyboard_LeftControl] =  KEY_LEFT_CONTROL,
  [Keyboard_Leftalt] =      KEY_LEFT_ALT,
  [Keyboard_LeftMeta] =     KEY_LEFT_SUPER,
  [Keyboard_RightShift] =   KEY_RIGHT_SHIFT,
  [Keyboard_RightControl] = KEY_RIGHT_CONTROL,
  [Keyboard_RightAlt] =     KEY_RIGHT_ALT,
  [Keyboard_RightMeta] =    KEY_RIGHT_SUPER,
  [Keyboard_Menu] =         KEY_KB_MENU,
  [Keyboard_LeftBracket] =  KEY_LEFT_BRACKET,
  [Keyboard_Backslash] =    KEY_BACKSLASH,
  [Keyboard_RightBracket] = KEY_RIGHT_BRACKET,
  [Keyboard_Grave] =        KEY_GRAVE,
  [Keyboard_Pad_0] =        KEY_KP_0,
  [Keyboard_Pad_1] =        KEY_KP_1,
  [Keyboard_Pad_2] =        KEY_KP_2,
  [Keyboard_Pad_3] =        KEY_KP_3,
  [Keyboard_Pad_4] =        KEY_KP_4,
  [Keyboard_Pad_5] =        KEY_KP_5,
  [Keyboard_Pad_6] =        KEY_KP_6,
  [Keyboard_Pad_7] =        KEY_KP_7,
  [Keyboard_Pad_8] =        KEY_KP_8,
  [Keyboard_Pad_9] =        KEY_KP_9,
  [Keyboard_Pad_Period] =   KEY_KP_DECIMAL,
  [Keyboard_Pad_Divide] =   KEY_KP_DIVIDE,
  [Keyboard_Pad_Multiply] = KEY_KP_MULTIPLY,
  [Keyboard_Pad_Minus] =    KEY_KP_SUBTRACT,
  [Keyboard_Pad_Add] =      KEY_KP_ADD,
  [Keyboard_Pad_Enter] =    KEY_KP_ENTER,
  [Keyboard_Pad_Equal] =    KEY_KP_EQUAL,
  [Mouse_Left_Button] =     MOUSE_LEFT_BUTTON,
  [Mouse_Right_Button] =    MOUSE_RIGHT_BUTTON
};

bool Backend_get_key_down(enum InputSource source) {
  return IsKeyDown(_input_source_to_raylib[source]);
}

bool Backend_get_mouse_button_down(enum InputSource source) {
  return IsMouseButtonDown(_input_source_to_raylib[source]);
}

float Backend_get_mouse_position_x(void) {
  return GetMouseX();
}

float Backend_get_mouse_position_y(void) {
  return GetMouseY();
}

float Backend_get_time(void) {
  return GetTime();
}

float Backend_get_frame_time(void) {
  return GetFrameTime();
}

void BackendImage_load(struct BackendImage * image, const char * filename) {
  Texture2D texture = LoadTexture(filename);
  image->width = texture.width;
  image->height = texture.height;
  image->depth = 1;
  image->levels = texture.mipmaps;
  image->layers = 1;
  image->id = texture.id;
  image->internal_format = texture.format;
}

void BackendImage_unload(struct BackendImage * image) {
  Texture2D texture;
  texture.width = image->width;
  texture.height = image->height;
  texture.mipmaps = image->levels;
  texture.id = image->id;
  texture.format = image->internal_format;
  UnloadTexture(texture);
}

void BackendUIVertex_render(const struct BackendImage * image, struct BackendUIVertex * vertexes, uint32_t num_indexes, const uint32_t * indexes) {
  rlSetTexture(image != NULL ? image->id : 0);

  rlBegin(RL_QUADS);
  for(uint32_t i = 0; i < num_indexes; ) {
    for(uint32_t j = 0; j < 3; j++, i++) {
      uint32_t index = indexes[i];
      rlColor4f(
          vertexes[index].rgba[0]
        , vertexes[index].rgba[1]
        , vertexes[index].rgba[2]
        , vertexes[index].rgba[3]
        );
      rlTexCoord2f(
          vertexes[index].st[0]
        , vertexes[index].st[1]
        );
      rlVertex2f(
          vertexes[index].xy[0]
        , vertexes[index].xy[1]
        );
      if(j == 0) {
        // stupid fucking raylib, honestly wtf
        rlTexCoord2f(
            vertexes[index].st[0]
          , vertexes[index].st[1]
          );
        rlVertex2f(
            vertexes[index].xy[0]
          , vertexes[index].xy[1]
          );
      }
    }
  }
  rlEnd();
}

static uint32_t _screen_width;
static uint32_t _screen_height;

void Backend_begin_rendering(uint32_t screen_width, uint32_t screen_height) {
  _screen_width = screen_width;
  _screen_height = screen_height;

  BeginDrawing();
  ClearBackground(alias_Color_to_raylib_Color(alias_Color_RAYWHITE));
}

void Backend_end_rendering(void) {
  EndDrawing();
}

void Backend_begin_2d(struct BackendMode2D mode) {
  int x = alias_pga2d_point_x(mode.viewport_min) * _screen_width;
  int y = alias_pga2d_point_y(mode.viewport_min) * _screen_height;
  int width = (alias_pga2d_point_x(mode.viewport_max) * _screen_width) - x;
  int height = (alias_pga2d_point_y(mode.viewport_max) * _screen_height) - y;

  alias_pga2d_Point center = alias_pga2d_sandwich_bm(alias_pga2d_point(0, 0), mode.camera);

  BeginMode2D((Camera2D) {
      .offset = { width / 2.0f, height / 2.0f }
    , .target = { alias_pga2d_point_x(center), alias_pga2d_point_y(center) }
    , .rotation = 0.0f
    , .zoom = mode.zoom
  });

  ClearBackground(alias_Color_to_raylib_Color(mode.background));
}

void Backend_end_2d(void) {
  EndMode2D();
}
