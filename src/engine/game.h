#pragma once

#include <raylib.h>
#include <alias/ecs.h>

typedef alias_ecs_EntityHandle Entity;

struct GameState {
  Camera2D camera;
  Entity player;
};

extern struct GameState game_state;
