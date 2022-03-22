#pragma once

#include "engine.h"

void Backend_init_window(uint32_t width, uint32_t height, const char * title);
void Backend_set_target_fps(uint32_t fps);
bool Backend_should_exit(void);

bool Backend_get_key_down(enum InputSource source);
bool Backend_get_mouse_button_down(enum InputSource source);
float Backend_get_mouse_position_x(void);
float Backend_get_mouse_position_y(void);
float Backend_get_time(void);
float Backend_get_frame_time(void);

struct BackendImage {
  uint32_t width;
  uint32_t height;
  uint32_t depth;

  uint32_t levels;
  uint32_t layers;

  uint64_t image;
  uint64_t memory;
  uint64_t imageview;

  uint32_t internal_format;
};

void BackendImage_load(struct BackendImage * image, const char * filename);
void BackendImage_unload(struct BackendImage * image);

struct BackendUIVertex {
  float xy[2];
  float rgba[4];
  float st[2];
};

struct BackendMode2D {
  alias_pga2d_Point viewport_min;
  alias_pga2d_Point viewport_max;
  alias_pga2d_Motor camera;
  alias_R           zoom;
  alias_Color       background;
};

void BackendUIVertex_render(const struct BackendImage * image, struct BackendUIVertex * vertexes, uint32_t num_indexes, const uint32_t * indexes);

void Backend_begin_rendering(uint32_t screen_width, uint32_t screen_height);
void Backend_end_rendering(void);

void Backend_begin_2d(struct BackendMode2D mode);
void Backend_end_2d(void);
