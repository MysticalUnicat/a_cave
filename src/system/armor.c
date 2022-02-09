#include "../component.h"

QUERY( armor_system
  , write(Armor, armor)
  , action(
    alias_R max = GameValue_get(&armor->max);
    alias_R delta = GameValue_get(&armor->regen_per_second) + max * GameValue_get(&armor->regen_percent_per_second);
    LiveValue_update(&armor->live, delta, max);
  )
)

