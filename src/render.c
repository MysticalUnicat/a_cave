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
    alias_R
        hw = r->width / 2
      , hh = r->height / 2
      , bl = -hw
      , br =  hw
      , bt = -hh
      , bb =  hh
      ;

    alias_Point2D box[] = {
        { br, bb }
      , { br, bt }
      , { bl, bt }
      , { bl, bb }
      };

    box[0] = alias_multiply_Affine2D_Point2D(t->value, box[0]);
    box[1] = alias_multiply_Affine2D_Point2D(t->value, box[1]);
    box[2] = alias_multiply_Affine2D_Point2D(t->value, box[2]);
    box[3] = alias_multiply_Affine2D_Point2D(t->value, box[3]);

    DrawTriangle(*(Vector2*)&box[0], *(Vector2*)&box[1], *(Vector2*)&box[2], r->color);
    DrawTriangle(*(Vector2*)&box[0], *(Vector2*)&box[2], *(Vector2*)&box[3], r->color);
  }

  QUERY(
      ( read, alias_LocalToWorld2D, t )
    , ( read, DrawCircle, c )
  ) {
    DrawCircle(t->value._13, t->value._23, c->radius, c->color);
  }

  EndDrawing();
}

