#include "../component.h"

QUERY( _player_control_movement
  , write(Movement, move)
  , write(PlayerControlMovement, controller)
  , action(
    alias_R
      dir_x = controller->inputs->left.boolean - controller->inputs->right.boolean,
      dir_y = controller->inputs->down.boolean - controller->inputs->up.boolean;

    move->target = move->done ? MovementTarget_None : move->target;
    move->movement_speed = 500;

    if(!alias_R_is_zero(dir_x) || !alias_R_is_zero(dir_y)) {
      move->target = MovementTarget_WorldDirection;
      move->target_direction = alias_pga2d_direction(dir_x, dir_y);
    } else if(controller->inputs->mouse_left_down.boolean) {
      move->done = false;
      move->target = MovementTarget_Point;
      move->target_point = controller->inputs->mouse_position.point;

      printf("move to %g %g\n", move->target_point.e02, move->target_point.e01);
    }
  )
)

void player_movement_system(void) {
  _player_control_movement();
}
