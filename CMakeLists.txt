cmake_minimum_required(VERSION 3.1)
# - 3.1 for C_STANDARD

project(a_cave LANGUAGES C)

set(BACKEND RAYLIB CACHE STRING "backend to use")

add_subdirectory(ext/alias)
add_subdirectory(ext/libuv)

if(${BACKEND} STREQUAL "RAYLIB")
  add_subdirectory(ext/raylib)
  set(BACKEND_LIBRARIES raylib)
  set(BACKEND_SOURCE src/engine/backend_raylib.c)
elseif(${BACKEND} STREQUAL "GLFW")
  set(BUILD_SHARED_LIBS   OFF      CACHE BOOL   "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF      CACHE BOOL   "" FORCE)
  set(GLFW_BUILD_TESTS    OFF      CACHE BOOL   "" FORCE)
  set(GLFW_BUILD_DOCS     OFF      CACHE BOOL   "" FORCE)
  set(GLFW_INSTALL        OFF      CACHE BOOL   "" FORCE)
  set(GLFW_LIBRARY_TYPE   "STATIC" CACHE STRING "" FORCE)

  add_subdirectory(ext/glfw)

  add_library(glad ext/glad/src/gl.c ext/glad/src/vulkan.c)
  target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/include)

  set(BACKEND_LIBRARIES glfw glad)
  set(BACKEND_SOURCE src/engine/backend_glfw.c)
else()
  message(FATAL_ERROR "set BACKEND to be either RAYLIB or GLFW")
endif()

add_library(a_engine
  src/engine/cbuf.c
  src/engine/engine.c

  ${BACKEND_SOURCE}
)
target_link_libraries(a_engine alias ${BACKEND_LIBRARIES} uv_a)

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
