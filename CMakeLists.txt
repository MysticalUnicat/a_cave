cmake_minimum_required(VERSION 3.1)
# - 3.1 for C_STANDARD

project(a_cave LANGUAGES C)

add_subdirectory(ext/alias)
add_subdirectory(ext/raylib)

add_library(a_engine
  src/engine/cbuf.c
  # src/engine/game.c
  # src/engine/render.c
  # src/engine/stats.c
  # src/engine/texture.c
  # src/engine/transform.c
  # src/engine/physics.c
  src/engine/engine.c
)
target_link_libraries(a_engine alias raylib)

add_executable(a_cave src/main.c)
target_link_libraries(a_cave a_engine)
