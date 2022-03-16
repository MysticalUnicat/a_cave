#include "engine.h"

#include "backend.h"

#include <alias/ui.h>
#include <alias/data_structure/inline_list.h>
#include <alias/data_structure/vector.h>
#include <alias/log.h>

#include <uchar.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static struct {
  GLFWwindow * window;

  uint32_t width;
  uint32_t height;
} _;

bool Vulkan_init(uint32_t * width, uint32_t * height, GLFWwindow * window);
void Vulkan_cleanup(void);

void Backend_init_window(uint32_t width, uint32_t height, const char * title) {
  if(!glfwInit()) {
    return;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  _.window = glfwCreateWindow(width, height, title, NULL, NULL);
  if(!_.window) {
    glfwTerminate();
    return;
  }

  if(!Vulkan_init(&width, &height, _.window)) {
    ALIAS_ERROR("failed to initialize vulkan backend");
    return;
  }
}

void Backend_cleanup(void) {
  glfwDestroyWindow(_.window);
  glfwTerminate();

  Vulkan_cleanup();
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

