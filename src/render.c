#include "render.h"

DEFINE_COMPONENT(DrawRectangle)

DEFINE_COMPONENT(DrawCircle)

void render_init(void) {
}

void render_frame(void) {
  BeginDrawing();

  ClearBackground(RAYWHITE);

  QUERY(
      ( read, Transform2D, t )
    , ( read, DrawRectangle, r )
  ) {
    DrawRectangle(t->position.x - r->width / 2.0f, t->position.y - r->height / 2.0f, r->width, r->height, r->color);
  }

  QUERY(
      ( read, Transform2D, t )
    , ( read, DrawCircle, c )
  ) {
    DrawCircle(t->position.x, t->position.y, c->radius, c->color);
  }

  EndDrawing();
}
