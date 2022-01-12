#include "../component.h"

QUERY( movement_system
  , write(Movement, move)
  , write(alias_Physics2DBodyMotion, body)
  , read(alias_LocalToWorld2D, local_to_world)
  , action(
    alias_pga2d_Motor M = local_to_world->motor;
    alias_pga2d_Point P = local_to_world->position;
    alias_pga2d_Point T;
    alias_pga2d_Direction D;
    alias_R S = move->movement_speed;

    if(move->target == MovementTarget_None) {
      return;
    }

    if(move->target == MovementTarget_LocalDirection) {
      move->done = true;
      D = move->target_direction;
    } else if(move->target == MovementTarget_WorldDirection) {
      move->done = true;
      D = move->target_direction;
    } else {
      if(move->done) {
        return;
      }

      if(move->target == MovementTarget_Point) {
        T = move->target_point;
      } else {
        const alias_LocalToWorld2D * tgt = alias_LocalToWorld2D_read(move->target_entity);
        if(tgt == NULL) {
          move->done = true;
        } else {
          T = tgt->position;
        }
      }

      D = alias_pga2d_sub_bb(T, P);
    }

    alias_R D_norm = alias_pga2d_inorm(alias_pga2d_b(D));
    if(D_norm <= alias_R_EPSILON) {
      move->done = true;
      return;
    }
    D = alias_pga2d_mul_bs(D, alias_R_ONE / D_norm);
   
    alias_pga2d_Vector F = alias_pga2d_dual(alias_pga2d_mul_bs(D, S));

    printf("P(%2ge01 %2ge02 %2ge12(a), D %2ge01(y) %2ge02(-x) %2ge12(a), D_norm %2g\n", P.e01, P.e02, P.e12, D.e01, D.e02, D.e12, D_norm);
    
    body->forque = alias_pga2d_add_vv(body->forque, F);
  )
)

