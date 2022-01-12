#include "local.h"

extern struct State playing_state;
extern struct State ui_demo_state;

static const char * _buttons[] = {
    "Start"
  , "UI Demo"
  , "Quit"
};

static uint32_t _current_button = 0;

void _main_menu_frame(void * ud) {
  (void)ud;

  if(main_inputs.menu_back.boolean) {
    Engine_pop_state();
    return;
  }

  if(main_inputs.menu_forward.boolean) {
    switch(_current_button) {
    case 0:
      Engine_pop_state();
      Engine_push_state(&playing_state);
      break;
    case 1:
      Engine_push_state(&ui_demo_state);
      break;
    case 2:
      Engine_pop_state();
      break;
    }
    return;
  }

  if(main_inputs.menu_down

  Engine_ui_center();
    Engine_ui_font_color(alias_Color_GRAY);

    Engine_ui_vertical();
      for(uint32_t i = 0; i < sizeof(_buttons)/sizeof(_buttons[0]); i++) {
        Engine_ui_text("%s %s", _current_button == i ? ">" : " ", _buttons[i]);
      }
    Engine_ui_end();

  Engine_ui_top();
  Engine_ui_font_color(alias_Color_BLACK);
  Engine_ui_text("version 1");
}

struct State intro_state = {
  .frame = _intro_frame
};
