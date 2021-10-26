#pragma once

#include "playing.c"

void _intro_frame(void * ud) {
  (void)ud;

  if(menu_back.value) {
    Engine_pop_state();
    return;
  }

  if(menu_forward.value) {
    Engine_pop_state();
    Engine_push_state(&playing_state);
    return;
  }

  Engine_ui_center();
    Engine_ui_font_color(alias_Color_GRAY);
    //Engine_ui_vertical();
      Engine_ui_font_size(40);
      Engine_ui_text("alias town");

    //  Engine_ui_font_size(20);
    //  Engine_ui_text("a game By Unicat");
    //Engine_ui_end();
}

struct State intro_state = {
  .frame = _intro_frame
};
