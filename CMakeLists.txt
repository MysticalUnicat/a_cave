cmake_minimum_required(VERSION 3.1)
# - 3.1 for C_STANDARD

project(a_cave LANGUAGES C)

# for mingw builds
if(WIN32)
  set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static")
endif()

# build as static as possible
set(BUILD_SHARED_LIBS   OFF      CACHE BOOL   "" FORCE)

# Alias (functionality is moved from the engine to Alias)
set(ALIAS_PROFILING ON CACHE BOOL "" FORCE)
add_subdirectory(ext/alias)

# libuv
add_subdirectory(ext/libuv)

# Vulkan headers
ExternalProject_Add(
  Vulkan-Headers
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/ext/Vulkan-Headers
  GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
  INSTALL_COMMAND ""
)

# GLSL compiler
ExternalProject_Add(
  glslangValidator
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/ext/glslang
  GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
  INSTALL_COMMAND ""
)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF      CACHE BOOL   "" FORCE)
set(GLFW_BUILD_TESTS    OFF      CACHE BOOL   "" FORCE)
set(GLFW_BUILD_DOCS     OFF      CACHE BOOL   "" FORCE)
set(GLFW_INSTALL        OFF      CACHE BOOL   "" FORCE)
set(GLFW_LIBRARY_TYPE   "STATIC" CACHE STRING "" FORCE)
add_subdirectory(ext/glfw)

# engine data
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/engine_ui_vert.o
	COMMAND glslangValidator -V -o engine_ui_vert.spv ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/backend_vk_ui.vert
  COMMAND ${CMAKE_LINKER} --relocatable --format binary --output engine_ui_vert.o engine_ui_vert.spv
	DEPENDS src/engine/backend_vk_ui.vert
  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/engine_ui_vert.spv
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/engine_ui_frag.o
	COMMAND glslangValidator -V -o engine_ui_frag.spv ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/backend_vk_ui.frag
  COMMAND ${CMAKE_LINKER} --relocatable --format binary --output engine_ui_frag.o engine_ui_frag.spv
	DEPENDS src/engine/backend_vk_ui.frag
  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/engine_ui_frag.spv
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# the engine as a library based on Alias (functionality is moved from games to engine)
add_library(a_engine
  src/engine/cbuf.c
  src/engine/engine.c
  src/engine/image.c

  src/engine/backend_glfw.c
  src/engine/backend_vk.c

  ${CMAKE_CURRENT_BINARY_DIR}/engine_ui_vert.o
  ${CMAKE_CURRENT_BINARY_DIR}/engine_ui_frag.o
)
target_link_libraries(a_engine alias glfw uv_a)
target_include_directories(a_engine PRIVATE ext/stb ${CMAKE_CURRENT_BINARY_DIR}/ext/Vulkan-Headers/include)

# the game
add_executable(alias_town
  src/main.c

	src/component.c

  src/prefab/camera.c
  src/prefab/grass.c
  src/prefab/player.c
  src/prefab/target.c

  src/state/intro.c
  src/state/paused.c
  src/state/playing.c
  src/state/main_menu.c
  src/state/ui_demo.c

	src/system/armor.c
  src/system/movement.c
  src/system/player_movement.c
	src/system/power.c
	src/system/shield.c
)
target_link_libraries(alias_town a_engine)
