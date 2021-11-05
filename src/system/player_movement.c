#include "../component.h"

QUERY( _player_control_movement
  , write(Movement, move)
  , write(PlayerControlMovement, controller)
  , action(
    alias_R
      dir_x = controller->left.value - controller->right.value,
      dir_y = controller->down.value - controller->up.value;

    if(alias_R_is_zero(dir_x) && alias_R_is_zero(dir_y)) {
      move->target = MovementTarget_None;
      return;
    }

    move->target = MovementTarget_Direction;
    move->target_direction = alias_pga2d_direction(dir_x, dir_y);
    move->movement_speed = 500;
  )
)

void player_movement_system(void) {
  _player_control_movement();
}
