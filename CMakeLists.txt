cmake_minimum_required(VERSION 3.1)
# - 3.1 for C_STANDARD

project(a_cave LANGUAGES C)

add_subdirectory(ext/alias)
add_subdirectory(ext/raylib)

add_executable(a_cave
  src/main.c
  src/cbuf.c
  src/texture.c
  src/stats.c
  src/game.c
)

target_link_libraries(a_cave alias raylib)

