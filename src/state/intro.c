#include "local.h"

extern struct State playing_state;

static int _intro_l = 0;

char * _unicat_names[] = {
  "Sarah",
  "Charlie",
  "Frank",
  "Anna",
  "Willa",
  "Melonie"
};

struct Image _intro_title_image = { .path = "intro_title.png" };

void _intro_frame(void * ud) {
  (void)ud;

  if(main_inputs.menu_back.boolean) {
    Engine_pop_state();
    return;
  }

  if(main_inputs.menu_forward.boolean) {
    Engine_pop_state();
    Engine_push_state(&playing_state);
    return;
  }

  Engine_ui_center();
    Engine_ui_image(&_intro_title_image);

  Engine_ui_center();
    Engine_ui_font_color(alias_Color_GRAY);

    Engine_ui_vertical();
      Engine_ui_font_size(40);
      Engine_ui_text("Alias Town");

      Engine_ui_font_size(20);
      Engine_ui_text("A game by %s | Unicat", _unicat_names[(_intro_l++ >> 5) % (sizeof(_unicat_names)/sizeof(_unicat_names[0]))]);
    Engine_ui_end();

  Engine_ui_top();
  Engine_ui_font_color(alias_Color_BLACK);
  Engine_ui_text("version 1");
}

struct State intro_state = {
  .frame = _intro_frame
};
