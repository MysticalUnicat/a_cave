#include "../component.h"

QUERY( movement_system
  , read(Movement, move)
  , read(alias_LocalToWorld2D, local_to_world)
  , write(alias_Physics2DBodyMotion, body)
  , action(
    alias_pga2d_Bivector force_line = alias_pga2d_direction(0, 0);
    alias_pga2d_Point position = local_to_world->position;
    const alias_LocalToWorld2D * tgt;

    switch(move->target) {
    case MovementTarget_None:
      break;
    case MovementTarget_Direction:
      force_line = alias_pga2d_grade_2(alias_pga2d_sandwich(alias_pga2d_b(move->target_direction), alias_pga2d_reverse_m(local_to_world->motor)));
      break;
    case MovementTarget_Point:
      force_line = alias_pga2d_ideal_normalized(alias_pga2d_sub_bb(position, move->target_point));
      break;
    case MovementTarget_Entity:
      tgt = alias_LocalToWorld2D_read(move->target_entity);
      force_line = alias_pga2d_ideal_normalized(alias_pga2d_sub_bb(position, tgt->position));
      break;
    }

    body->forque = alias_pga2d_add(
      alias_pga2d_v(body->forque),
      alias_pga2d_mul(alias_pga2d_s(move->movement_speed), alias_pga2d_dual_b(force_line)));
  )
)
