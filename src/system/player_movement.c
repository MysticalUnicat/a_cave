#include "../component.h"

QUERY( _player_control_movement
  , write(Movement, move)
  , write(PlayerControlMovement, controller)
  , action(
    alias_R
      right_left = controller->inputs->right.boolean - controller->inputs->left.boolean,
      up_down    = controller->inputs->down.boolean - controller->inputs->up.boolean;

    move->target = move->done ? MovementTarget_None : move->target;
    move->movement_speed = 500;

    {
      struct alias_Translation2D * translation = alias_Translation2D_write(controller->target);
      if(translation) {
        translation->value.e01 = controller->inputs->mouse_position.point.e02;
        translation->value.e02 = -controller->inputs->mouse_position.point.e01;
        translation->value.e12 = alias_R_ZERO;
      }
    }

    if(!alias_R_is_zero(right_left) || !alias_R_is_zero(up_down)) {
      move->target = MovementTarget_WorldDirection;
      move->target_direction.e01 = right_left;
      move->target_direction.e02 = up_down;
      move->target_direction.e12 = alias_R_ZERO;
    } else if(controller->inputs->mouse_left_down.boolean) {
      move->done = false;
      move->target = MovementTarget_Point;
      move->target_point.e01 = controller->inputs->mouse_position.point.e01;
      move->target_point.e02 = controller->inputs->mouse_position.point.e02;
      move->target_point.e12 = alias_R_ONE;
    }
  )
)

void player_movement_system(void) {
  _player_control_movement();
}
