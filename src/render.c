#include "render.h"

DEFINE_COMPONENT(DrawRectangle)

DEFINE_COMPONENT(DrawCircle)

void render_init(void) {
}

void render_frame(void) {
  BeginDrawing();

  ClearBackground(BLACK);

  QUERY(
      ( read, Transform2D, t )
    , ( read, DrawRectangle, r )
  ) {
    DrawRectangle(t->x - r->width / 2.0f, t->y - r->height / 2.0f, r->width, r->height, r->color);
  }

  QUERY(
      ( read, Transform2D, t )
    , ( read, DrawCircle, c )
  ) {
    DrawCircle(t->x, t->y, c->radius, c->color);
  }

  EndDrawing();
}
