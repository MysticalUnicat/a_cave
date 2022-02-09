#include "../component.h"

QUERY( power_system
  , write(Power, power)
  , read(Shield, shield)
  , optional(Shield)
  , action(
    alias_R max = GameValue_get(&power->max);
    alias_R delta = GameValue_get(&power->generation_per_second);
    if(shield && shield->recharging) {
      delta -= GameValue_get(&shield->recharge_power_per_second);
    }
    LiveValue_update(&power->live, delta, max);
  )
)

