#include "engine.h"

#include "backend.h"

#include <alias/ui.h>
#include <alias/data_structure/inline_list.h>

#include <uchar.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

static struct {
  GLFWwindow * window;




  uint32_t width;
  uint32_t height;
} _;

struct PerFrameBuffer {
  uint32_t index;
  float real_time;
  float real_time_delta;
};

struct PerViewBuffer2D {
  alias_pga2d_Motor; // vec4
  float game_time;
  float game_time_delta;
};

struct PerDrawBuffer {
  int placeholder;
};

#define SHADER_HEADER                              \
  "#version 640\n"                                 \
  "layout(std430, binding = 0) uniform uFRAME {\n" \
  "  int index;\n"                                 \
  "  float real_time;\n"                           \
  "  float real_time_delta;\n"                     \
  "};\n"                                           \
  "layout(std430, binding = 1) uniform uVIEW {\n"  \
  "  mat4 camera;\n"                               \
  "  float game_time;\n"                           \
  "  float game_time_delta;\n"                     \
  "};\n"                                           \
  "layout(std430, binding = 2) uniform uDRAW {\n"  \
  "  int placeholder;\n"                           \
  "};\n"

#define VERTEX_SHADER_HEADER \
  SHADER_HEADER

#define FRAGMENT_SHADER_HEADER                         \
  SHADER_HEADER                                        \
  "layout(binding = 3) uniform sampler2D texture_0;\n" \
  "layout(location = 0) out vec4 out_color;\n"

const char basic_vertex_shader[] =
  VERTEX_SHADER_HEADER
  "layout(location = 0) in vec2 in_position;\n"
  "layout(location = 1) in vec2 in_texcoord;\n"
  "layout(location = 2) in vec4 in_color;\n"
  "layout(location = 0) out vec2 out_texcoord;\n"
  "layout(location = 1) out vec4 out_color;\n"
  "void main() {\n"
  "  out_texcoord = in_texcoord;\n"
  "  out_color = in_color;\n"
  "  gl_Position = uVIEW.camera * vec4(in_position, 1);\n"
  "}\n"
  ;

const char basic_fragment_shader[] =
  FRAGMENT_SHADER_HEADER
  "layout(location = 0) in vec2 in_texcoord;\n"
  "layout(location = 1) in vec4 in_color;\n"
  "void main() {\n"
  "  out_color = texture(texture_0, in_texcoord) * in_color;\n"
  "}\n"
  ;

void Backend_init_window(uint32_t width, uint32_t height, const char * title) {
  if(!glfwInit()) {
    return;
  }

  _.window = glfwCreateWindow(width, height, title, NULL, NULL);
  if(!_.window) {
    glfwTerminate();
    return;
  }

  glfwMakeContextCurrent(_.window);

  gladLoadGL(glfwGetProcAddress);


}

void Backend_set_target_fps(uint32_t fps) {
}

bool Backend_should_exit(void) {
  return _.window == NULL || glfwWindowShouldClose(_.window);
}

bool Backend_get_key_down(enum InputSource source) {
}

bool Backend_get_mouse_button_down(enum InputSource source) {
}

float Backend_get_mouse_position_x(void) {
}

float Backend_get_mouse_position_y(void) {
}

float Backend_get_time(void) {
  return glfwGetTime();
}

float Backend_get_frame_time(void) {
}

void BackendImage_load(struct BackendImage * image, const char * filename) {
}

void BackendImage_unload(struct BackendImage * image) {
}

void BackendUIVertex_render(const struct BackendImage * image, struct BackendUIVertex * vertexes, uint32_t num_indexes, const uint32_t * indexes) {
}

void Backend_begin_rendering(uint32_t screen_width, uint32_t screen_height) {
  _.width = screen_width;
  _.height = screen_height;
}

void Backend_end_rendering(void) {
  glfwSwapBuffers(_.window);
  glfwPollEvents();
}

void Backend_begin_2d(struct BackendMode2D mode) {
  glClear(GL_COLOR_BUFFER_BIT);
}

void Backend_end_2d(void) {
}
