#include "render.h"

DEFINE_COMPONENT(DrawRectangle)

DEFINE_COMPONENT(DrawCircle)

void render_init(void) {
}

void render_frame(void) {
  BeginDrawing();

  ClearBackground(RAYWHITE);

  QUERY(
      ( read, alias_LocalToWorld2D, t )
    , ( read, DrawRectangle, r )
  ) {
    DrawRectangle(t->value._13 - r->width / 2.0f, t->value._23 - r->height / 2.0f, r->width, r->height, r->color);
  }

  QUERY(
      ( read, alias_LocalToWorld2D, t )
    , ( read, DrawCircle, c )
  ) {
    DrawCircle(t->value._13, t->value._23, c->radius, c->color);
  }

  EndDrawing();
}

