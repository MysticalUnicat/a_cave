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

add_executable(a_cave
  src/cbuf.c
  src/game.c
  src/main.c
  src/physics.c
  src/render.c
  src/stats.c
  src/texture.c
  src/transform.c
)

target_link_libraries(a_cave alias raylib chipmunk_static)

