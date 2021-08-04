#pragma once

//#include "input.c"

//#include "start.c"

struct {
  Entity text;
} _intro;

void _intro_begin(void * ud) {
  (void)ud;

  _intro.text = SPAWN(
      ( alias_Translation2D, .value.x = SCREEN_WIDTH / 3, .value.y = SCREEN_HEIGHT/2 - 40 )
    , ( DrawText, .text = "A CAVE", .size = 40, .color = GRAY )
    );
}

void _intro_frame(void * ud) {
  //extern struct State playing;

  if(menu_back.value) {
    Engine_pop_state();
    return;
  }

  if(menu_forward.value) {
    Engine_pop_state();
    //Engine_push_state(&start_state);
    return;
  }

  //DrawText("A CAVE", SCREEN_WIDTH / 2 - MeasureText("A CAVE", 40) / 2, SCREEN_HEIGHT/2 - 40, 40, GRAY);
}

void _intro_end(void * ud) {
  alias_ecs_despawn(Engine_ecs(), 1, &_intro.text);
}

struct State intro_state = {
    .begin = _intro_begin
  , .frame = _intro_frame
  , .end   = _intro_end
};
