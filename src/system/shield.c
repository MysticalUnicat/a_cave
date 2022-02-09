#include "../component.h"

QUERY( shield_system
  , write(Shield, shield)
  , action(
    alias_R max = GameValue_get(&shield->max);
    alias_R delta = GameValue_get(&shield->regen_per_second) + max * GameValue_get(&shield->regen_percent_per_second);

    shield->recharging = shield->live.current < GameValue_get(&shield->max)
                      && Engine_time() > shield->live.last_damage_time + GameValue_get(&shield->recharge_delay)
                       ;

    if(shield->recharging) {
      delta += GameValue_get(&shield->recharge_per_second) + max * GameValue_get(&shield->recharge_percentage_per_second);
    }

    LiveValue_update(&shield->live, delta, max);
  )
)

