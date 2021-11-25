#include "../component.h"

QUERY( _player_control_movement
  , write(Movement, move)
  , write(PlayerControlMovement, controller)
  , action(
    alias_R
      dir_x = controller->inputs->left.boolean - controller->inputs->right.boolean,
      dir_y = controller->inputs->down.boolean - controller->inputs->up.boolean;

    printf("player movement 1\n");

    if(alias_R_is_zero(dir_x) && alias_R_is_zero(dir_y)) {
      move->target = MovementTarget_None;
      return;
    }

    printf("player movement 2\n");

    move->target = MovementTarget_Direction;
    move->target_direction = alias_pga2d_direction(dir_x, dir_y);
    move->movement_speed = 500;
  )
)

void player_movement_system(void) {
  _player_control_movement();
}
