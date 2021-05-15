cmake_minimum_required(VERSION 3.1)
# - 3.1 for C_STANDARD

project(a_cave LANGUAGES C)

add_subdirectory(ext/alias)
add_subdirectory(ext/raylib)

set(BUILD_DEMOS OFF)
set(BUILD_SHARED OFF)
set(INSTALL_STATIC OFF)
add_subdirectory(ext/Chipmunk2D)

target_include_directories(chipmunk_static PUBLIC ext/Chipmunk2D/include)

add_library(a_engine
  src/cbuf.c
  src/event.c
  src/game.c
  src/physics.c
  src/render.c
  src/state.c
  src/stats.c
  src/texture.c
  src/transform.c
)
target_link_libraries(a_engine alias raylib chipmunk_static)

add_executable(arkanoid demo/arkanoid.c)
target_link_libraries(arkanoid a_engine)

add_executable(asteroids demo/asteroids.c)
target_link_libraries(asteroids a_engine)

