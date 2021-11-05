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
    case MovementTarget_Point:
      tgt = alias_LocalToWorld2D_read(move->target_entity);
      force_line = alias_pga2d_ideal_normalized(alias_pga2d_sub_bb(position, tgt->position));
      break;
    }

    body->forque = alias_pga2d_add(
      alias_pga2d_v(body->forque),
      alias_pga2d_mul(alias_pga2d_s(move_speed), alias_pga2d_dual_b(force_line)));

    /*

    alias_R dir_x = _playing.player_left.value - _playing.player_right.value;
    alias_R dir_y = _playing.player_down.value - _playing.player_up.value;

    // link event to forque pushers?
    alias_Transform2D * transform;
    alias_Physics2DBodyMotion * body;
    alias_ecs_write_entity_component(Engine_ecs(), _playing.player, alias_Transform2D_component(), (void **)&transform);
    alias_ecs_write_entity_component(Engine_ecs(), _playing.player, alias_Physics2DBodyMotion_component(), (void **)&body);

    alias_pga2d_Motor position = transform->value;

    #if 0
    alias_pga2d_Point center = alias_pga2d_sandwich_bm(alias_pga2d_point(0, 0), transform->value);
    alias_pga2d_Point offset = alias_pga2d_add_bb(center, alias_pga2d_direction(dir_x, dir_y));
    alias_pga2d_Line force_line = alias_pga2d_join_bb(center, offset);

    body->forque = alias_pga2d_add(
        alias_pga2d_v(body->forque),
        alias_pga2d_mul(alias_pga2d_s(move_speed), alias_pga2d_v(force_line)));
    #else
    // fake force line
    alias_pga2d_Bivector force_line = (alias_pga2d_Bivector){ .e01 = dir_x, .e02 = dir_y };

    body->forque = alias_pga2d_add(
        alias_pga2d_v(body->forque),
        alias_pga2d_mul(alias_pga2d_s(move_speed),
                        alias_pga2d_dual(alias_pga2d_grade_2(alias_pga2d_sandwich(
                            alias_pga2d_b(force_line),
                            alias_pga2d_reverse_m(position))))));
    #endif

    //printf("playing, velocity = %g %g %g\n", limotion->value.e02, limotion->value.e01, limotion->value.e12);
    */
  )
)
