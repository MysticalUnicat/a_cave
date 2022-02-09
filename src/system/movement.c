#include "../component.h"

QUERY( movement_system
  , write(Movement, move)
  , write(alias_Physics2DBodyMotion, body)
  , read(alias_LocalToWorld2D, local_to_world)
  , action(
    // Xw = ~M Xb M
    alias_pga2d_Motor M = local_to_world->motor;
    
    alias_pga2d_Direction Dw = move->target_direction;
    
    alias_pga2d_AntiBivector Fb = { .e0 = 0, .e1 = 0, .e2 = 0 };

    if(move->target == MovementTarget_None) {
      return;
    }

    if(move->target == MovementTarget_LocalDirection) {
      move->done = true;
      Fb = alias_pga2d_dual_b(Dw);
    } else if(move->target == MovementTarget_WorldDirection) {
      move->done = true;
      Fb = alias_pga2d_dual(alias_pga2d_grade_2(alias_pga2d_sandwich(alias_pga2d_b(Dw), alias_pga2d_reverse_m(M))));
    } else {
      if(move->done) {
        return;
      }
      alias_pga2d_Point Tw;
      if(move->target == MovementTarget_Point) {
        Tw = move->target_point;
      } else {
        const alias_LocalToWorld2D * tgt = alias_LocalToWorld2D_read(move->target_entity);
        if(tgt == NULL) {
          move->done = true;
        } else {
          Tw = tgt->position;
        }
      }
      
      Fb = alias_pga2d_mul(
          alias_pga2d_s(-1.0f)
        , alias_pga2d_regressive_product(
            alias_pga2d_sandwich(alias_pga2d_b(Tw), alias_pga2d_reverse_m(M))
          , alias_pga2d_b(alias_pga2d_point(0, 0))
          )
        );
    }

    // cap Fb's magnitude
    alias_R Fn = alias_pga2d_norm(alias_pga2d_v(Fb));
    if(Fn >= alias_R_ONE) {
      Fb = alias_pga2d_mul_sv(1.0f / Fn, Fb);
    } else if(Fn <= 0.75f) {
      move->done = true;
    }

    body->forque = alias_pga2d_add_vv(body->forque, alias_pga2d_mul_sv(move->movement_speed, Fb));
  )
)

