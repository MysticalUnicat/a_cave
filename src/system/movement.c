#include "../component.h"

QUERY( movement_system
  , write(Movement, move)
  , write(alias_Physics2DBodyMotion, body)
  , read(alias_LocalToWorld2D, local_to_world)
  , action(
    alias_pga2d_Bivector force_line = alias_pga2d_direction(0, 0);
    alias_pga2d_Point position = local_to_world->position, target_position;
    const alias_LocalToWorld2D * tgt;
    alias_R speed = move->movement_speed;

    if(move->target == MovementTarget_None) {
      return;
    }

    if(move->target == MovementTarget_LocalDirection) {
      move->done = true;
      force_line = move->target_direction;
      force_line = alias_pga2d_grade_2(alias_pga2d_sandwich(alias_pga2d_b(force_line), alias_pga2d_reverse_m(local_to_world->motor)));
    } else if(move->target == MovementTarget_WorldDirection) {
      move->done = true;
      force_line = move->target_direction;
    } else {
      if(move->done) {
        return;
      }

      if(move->target == MovementTarget_Point) {
        target_position = move->target_point;
      } else {
        tgt = alias_LocalToWorld2D_read(move->target_entity);
        if(tgt == NULL) {
          move->done = true;
        } else {
          target_position = tgt->position;
        }
      }

      force_line = alias_pga2d_sub_bb(position, target_position);
    }

    alias_R distance = alias_pga2d_ideal_norm(alias_pga2d_b(force_line)).one;
    if(distance <= alias_R_EPSILON) {
      move->done = true;
      return;
    }

    force_line = alias_pga2d_mul(alias_pga2d_b(force_line), alias_pga2d_s(speed / distance));

    printf("position %g %g %g, moving %g %g %g\n", position.e02, position.e01, position.e12, force_line.e02, force_line.e01, force_line.e12);
    
    body->forque = alias_pga2d_add(alias_pga2d_v(body->forque), alias_pga2d_dual_b(force_line));
  )
)

