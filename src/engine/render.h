#pragma once

#include "util.h"
#include "transform.h"

DECLARE_COMPONENT(DrawRectangle, {
  float width;
  float height;
  Color color;
})

DECLARE_COMPONENT(DrawCircle, {
  float radius;
  Color color;
})

void render_init(void);
void render_frame(void);

