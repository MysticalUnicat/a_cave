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

# GLFW
set(GLFW_BUILD_EXAMPLES OFF      CACHE BOOL   "" FORCE)
set(GLFW_BUILD_TESTS    OFF      CACHE BOOL   "" FORCE)
set(GLFW_BUILD_DOCS     OFF      CACHE BOOL   "" FORCE)
set(GLFW_INSTALL        OFF      CACHE BOOL   "" FORCE)
set(GLFW_LIBRARY_TYPE   "STATIC" CACHE STRING "" FORCE)
add_subdirectory(ext/glfw)

# the engine as a library based on Alias (functionality is moved from games to engine)
add_library(a_engine
  src/engine/cbuf.c
  src/engine/engine.c
  src/engine/image.c

  src/engine/backend_glfw.c
  src/engine/backend_vk.c
  src/engine/glad_vk.c
)
target_link_libraries(a_engine alias glfw uv_a)
target_include_directories(a_engine PRIVATE ext/stb)

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
