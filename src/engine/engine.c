#include "engine.h"

#include "backend.h"

#include <alias/ui.h>
#include <alias/data_structure/inline_list.h>
#include <alias/data_structure/vector.h>

#include <uchar.h>

#define UI_NUM_VERTEXES (1024 * 1024)
#define UI_NUM_INDEXES  (1024 * 1024)
#define UI_NUM_GROUPS   1024

static uv_loop_t _loop;

static inline void _frame_timer_f(uv_timer_t * t);

static alias_ecs_Instance * _ecs;

static alias_ui * _ui;
static uint32_t _ui_indexes_data[UI_NUM_INDEXES];
static struct BackendUIVertex _ui_vertexes_data[UI_NUM_VERTEXES];
static alias_memory_SubBuffer _ui_indexes = {
    .pointer = _ui_indexes_data
  , .count = UI_NUM_INDEXES
  , .stride = sizeof(_ui_indexes_data[0])
  , .type_format = alias_memory_Format_Uint32
  , .type_length = 1
};
static alias_memory_SubBuffer _ui_vertexes_xy = {
    .pointer = (uint8_t *)_ui_vertexes_data + offsetof(struct BackendUIVertex, xy)
  , .count = UI_NUM_VERTEXES
  , .stride = sizeof(_ui_vertexes_data[0])
  , .type_format = alias_memory_Format_Float32
  , .type_length = 2
};
static alias_memory_SubBuffer _ui_vertexes_rgba = {
    .pointer = (uint8_t *)_ui_vertexes_data + offsetof(struct BackendUIVertex, rgba)
  , .count = UI_NUM_VERTEXES
  , .stride = sizeof(_ui_vertexes_data[0])
  , .type_format = alias_memory_Format_Float32
  , .type_length = 4
};
static alias_memory_SubBuffer _ui_vertexes_st = {
    .pointer = (uint8_t *)_ui_vertexes_data + offsetof(struct BackendUIVertex, st)
  , .count = UI_NUM_VERTEXES
  , .stride = sizeof(_ui_vertexes_data[0])
  , .type_format = alias_memory_Format_Float32
  , .type_length = 2
};
static alias_ui_OutputGroup _ui_groups[UI_NUM_GROUPS];
static bool _ui_recording;

static alias_R _physics_speed;

static struct State * _current_state;

static uint32_t _screen_width;
static uint32_t _screen_height;

void Engine_init(uint32_t screen_width, uint32_t screen_height, const char * title, struct State * initial_state) {
  _screen_width = screen_width;
  _screen_height = screen_height;

  Backend_init_window(screen_width, screen_height, title);

  alias_ecs_create_instance(NULL, &_ecs);

  alias_ui_initialize(alias_default_MemoryCB(), &_ui);
  _ui_recording = false;

  _physics_speed = alias_R_ONE;
  _current_state = NULL;

  Engine_push_state(initial_state);
}

static void _update_input(void);
static void _update_events(void);
static bool _update_state(void);
static void _update_physics(void);
static void _update_display(void);

static bool _update(void) {
  _update_physics();
  _update_display();
  if(Backend_should_exit()) {
    return false;
  }
  _update_input();
  _update_events();
  return _update_state();
}

uv_loop_t * Engine_uv_loop(void) {
  return &_loop;
}

void Engine_run(void) {
  uv_timer_t _frame_timer;

  uv_loop_init(&_loop);

  uv_timer_init(&_loop, &_frame_timer);
  uv_timer_start(&_frame_timer, _frame_timer_f, 0, 16);
  
  uv_run(&_loop, UV_RUN_DEFAULT);
}

static inline void _frame_timer_f(uv_timer_t * t) {
  if(!_update()) {
    uv_stop(&_loop);
  }
}

// state
void Engine_push_state(struct State * state) {
  if(_current_state != NULL && _current_state->pause != NULL) {
    _current_state->pause(_current_state->ud);
  }
  state->prev = _current_state;
  _current_state = state;
  if(state->begin != NULL) {
    state->begin(state->ud);
  }
}

void Engine_pop_state(void) {
  if(_current_state == NULL) {
    return;
  }
  if(_current_state->end != NULL) {
    _current_state->end(_current_state->ud);
  }
  _current_state = _current_state->prev;
  if(_current_state != NULL && _current_state->unpause != NULL) {
    _current_state->unpause(_current_state->ud);
  }
}

static bool _update_state(void) {
  struct State * current = _current_state;
  if(current == NULL) {
    return false;
  }
  if(current != NULL && current->frame != NULL) {
    current->frame(current->ud);
  }
  while(current != NULL) {
    if(current->background != NULL) {
      current->background(current->ud);
    }
    current = current->prev;
  }
  return true;
}

// parameter access
alias_ecs_Instance * Engine_ecs(void) {
  return _ecs;
}

// input
static uint32_t _input_backend_pair_count = 0;
static const struct InputBackendPair * _input_backend_pairs = NULL;

static uint32_t _input_binding_count = 0;
static alias_R * _input_bindings = NULL;

void Engine_set_player_input_backend(uint32_t player_index, uint32_t pair_count, const struct InputBackendPair * pairs) {
  (void)player_index;
  _input_backend_pair_count = pair_count;
  _input_backend_pairs = pairs;

  uint32_t max_binding_index = 0;
  for(uint32_t i = 0; i < pair_count; i++) {
    if(pairs[i].binding > max_binding_index) {
      max_binding_index = pairs[i].binding;
    }
  }

  if(max_binding_index + 1 > _input_binding_count) {
    _input_bindings = alias_realloc(
        alias_default_MemoryCB()
      , _input_bindings
      , sizeof(*_input_bindings) * _input_binding_count
      , sizeof(*_input_bindings) * (max_binding_index + 1)
      , alignof(*_input_bindings)
      );
    _input_binding_count = max_binding_index + 1;
  }
}

static struct {
  uint32_t count;
  struct InputSignal * signals;
} _input_frontends[MAX_INPUT_FRONTEND_SETS] = { 0 };

uint32_t Engine_add_input_frontend(uint32_t player_index, uint32_t signal_count, struct InputSignal * signals) {
  (void)player_index;

  uint32_t index = 0;
  for(; index < MAX_INPUT_FRONTEND_SETS; index++) {
    if(_input_frontends[index].count == 0) {
      break;
    }
  }
  if(index < MAX_INPUT_FRONTEND_SETS) {
    _input_frontends[index].count = signal_count;
    _input_frontends[index].signals = signals;
    return index;
  }
  return -1;
}

void Engine_remove_input_frontend(uint32_t player_index, uint32_t index) {
  (void)player_index;

  if(index < MAX_INPUT_FRONTEND_SETS) {
    _input_frontends[index].count = 0;
    _input_frontends[index].signals = NULL;
  }
}

static void _update_input(void) {
  alias_memory_clear(_input_bindings, sizeof(*_input_bindings) * _input_binding_count);

  for(uint32_t i = 0; i < _input_backend_pair_count; i++) {
    enum InputSource source = _input_backend_pairs[i].source;
    alias_R * binding = &_input_bindings[_input_backend_pairs[i].binding];

    switch(source) {
    case Keyboard_Apostrophe ... Keyboard_Pad_Equal:
      if(Backend_get_key_down(source)) {
        *binding = alias_R_ONE;
      }
      break;
    case Mouse_Left_Button ... Mouse_Right_Button:
      if(Backend_get_mouse_button_down(source)) {
        *binding = alias_R_ONE;
      }
      break;
    case Mouse_Position_X:
      *binding = Backend_get_mouse_position_x();
      break;
    case Mouse_Position_Y:
      *binding = Backend_get_mouse_position_y();
      break;
    case InputSource_COUNT:
      break;
    }
  }

  for(uint32_t i = 0; i < MAX_INPUT_FRONTEND_SETS; i++) {
    for(uint32_t j = 0; j < _input_frontends[i].count; j++) {
      struct InputSignal * signal = &_input_frontends[i].signals[j];
      switch(signal->type) {
      case InputSignal_Pass:
        {
          signal->boolean = _input_bindings[signal->bindings[0]] > alias_R_ZERO;
          break;
        }
      case InputSignal_Up:
        {
          bool value = _input_bindings[signal->bindings[0]] > alias_R_ZERO;
          signal->boolean = !signal->internal.up && value;
          signal->internal.up = value;
          break;
        }
      case InputSignal_Down:
        {
          bool value = _input_bindings[signal->bindings[0]] > alias_R_ZERO;
          signal->boolean = signal->internal.down && !value;
          signal->internal.down = value;
          break;
        }
      case InputSignal_Direction:
        {
          signal->direction = alias_pga2d_direction(_input_bindings[signal->bindings[0]], _input_bindings[signal->bindings[1]]);
          break;
        }
      case InputSignal_Point:
        {
          signal->point = alias_pga2d_point(_input_bindings[signal->bindings[0]], _input_bindings[signal->bindings[1]]);
          break;
        }
      case InputSignal_ViewportPoint:
        {
          alias_R px = _input_bindings[signal->bindings[0]];
          alias_R py = _input_bindings[signal->bindings[1]];

          const struct Camera * camera = Camera_read(signal->click_camera);
          const struct alias_LocalToWorld2D * transform = alias_LocalToWorld2D_read(signal->click_camera);

          alias_R minx = alias_pga2d_point_x(camera->viewport_min) * _screen_width;
          alias_R miny = alias_pga2d_point_y(camera->viewport_min) * _screen_height;
          alias_R maxx = alias_pga2d_point_x(camera->viewport_max) * _screen_width;
          alias_R maxy = alias_pga2d_point_y(camera->viewport_max) * _screen_height;
          alias_R width = maxx - minx;
          alias_R height = maxy - miny;

          px -= minx;
          py -= miny;
          if(px < 0 || py < 0 || px > width || py > height) {
            break;
          }

          px -= width / 2;
          py -= height / 2;
          
          alias_R cx = alias_pga2d_point_x(transform->position);
          alias_R cy = alias_pga2d_point_y(transform->position);

          px += cx;
          py += cy;

          signal->point = alias_pga2d_point(px, py);
          
          break;
        }
      }
    }
  }
}

// events
DEFINE_COMPONENT(Event)

static uint32_t _events_id = 1;

uint32_t Engine_next_event_id(void) {
  return _events_id++;
}

QUERY( _update_events
  , state(uint32_t, last_highest_seen)
  , state(struct CmdBuf, cbuf)
  , state(uint32_t, next_highest_seen)
  , pre(
    CmdBuf_begin_recording(&state->cbuf);
    state->next_highest_seen = state->last_highest_seen;
  )
  , read(Event, e)
  , action(
    if(e->id <= state->last_highest_seen) {
      CmdBuf_despawn(&state->cbuf, entity);
    } else if(e->id > state->next_highest_seen) {
      state->next_highest_seen = e->id;
    }
  )
  , post(
    state->last_highest_seen = state->next_highest_seen;
    CmdBuf_end_recording(&state->cbuf);
    CmdBuf_execute(&state->cbuf, Engine_ecs());
  )
)

// transform
alias_TransformBundle * Engine_transform_bundle(void) {
  static alias_TransformBundle bundle;
  static bool init = false;
  if(!init) {
    alias_TransformBundle_initialize(Engine_ecs(), &bundle);
    init = true;
  }
  return &bundle;
}

// physics
alias_Physics2DBundle * Engine_physics_2d_bundle(void) {
  static alias_Physics2DBundle bundle;
  static bool init = false;
  if(!init) {
    alias_Physics2DBundle_initialize(Engine_ecs(), &bundle, Engine_transform_bundle());
    init = true;
  }
  return &bundle;
}

alias_R Engine_physics_speed(void) {
  return _physics_speed;
}

void Engine_set_physics_speed(alias_R speed) {
  _physics_speed = speed;
}

alias_R Engine_frame_time(void) {
  return Backend_get_frame_time();
}

alias_R Engine_time(void) {
  return Backend_get_time();
}

static void _update_physics(void) {
  const float timestep = 1.0f / 60.0f;

  static float p_time = 0.0f;
  static float s_time = 0.0f;

  s_time += Backend_get_frame_time() * _physics_speed;

  if(p_time >= s_time) {
    alias_transform_update2d_serial(Engine_ecs(), Engine_transform_bundle());
  }

  while(p_time < s_time) {
    alias_physics_update2d_serial_pre_transform(Engine_ecs(), Engine_physics_2d_bundle(), timestep);

    alias_transform_update2d_serial(Engine_ecs(), Engine_transform_bundle());

    alias_physics_update2d_serial_post_transform(Engine_ecs(), Engine_physics_2d_bundle(), timestep);

    p_time += timestep;
  }
}

// ====================================================================================================================
// Resource ===========================================================================================================
enum ResourceType {
    ResourceType_Invalid
  , ResourceType_Image
};

struct LoadedResource {
  alias_InlineList list;

  enum ResourceType type;
  uint32_t id, gen;

  union {
    struct BackendImage image;
  };
};

alias_InlineList _free_resources = ALIAS_INLINE_LIST_INIT(_free_resources);
alias_InlineList _inactive_resources = ALIAS_INLINE_LIST_INIT(_inactive_resources);
alias_InlineList _active_resources = ALIAS_INLINE_LIST_INIT(_active_resources);
alias_Vector(struct LoadedResource *) _resources = ALIAS_VECTOR_INIT;

static uint32_t _resource_id = 1, _resource_gen = 1;

void _free_resource(struct LoadedResource * resource) {
  switch(resource->type) {
  case ResourceType_Image:
    BackendImage_unload(&resource->image);
    break;
  }

  alias_InlineList_remove_self(&resource->list);
  alias_InlineList_push(&_free_resources, &resource->list);
  _resources.data[resource->id - 1] = NULL;
}

void _resources_gc(void) {
  struct LoadedResource * resource, * next_resource;
  ALIAS_INLINE_LIST_EACH_CONTAINER_SAFE(&_inactive_resources, resource, next_resource, list) {
    _free_resource(resource);
  }
  alias_InlineList_push_list(&_inactive_resources, &_active_resources);
  _resource_gen++;
}

struct LoadedResource * _allocate_resource(enum ResourceType type) {
  struct LoadedResource * resource;
  if(alias_InlineList_is_empty(&_free_resources)) {
    resource = alias_malloc(alias_default_MemoryCB(), sizeof(*resource), alignof(*resource));
    alias_InlineList_init(&resource->list);
    resource->id = _resource_id++;

    alias_Vector_space_for(&_resources, alias_default_MemoryCB(), 1);
    *alias_Vector_push(&_resources) = NULL;
  } else {
    resource = ALIAS_INLINE_LIST_CONTAINER(alias_InlineList_pop(&_free_resources), struct LoadedResource, list);
  }
  alias_InlineList_push(&_active_resources, &resource->list);
  resource->type = type;
  resource->gen = _resource_gen;
  _resources.data[resource->id - 1] = resource;
  return resource;
}

void _touch_resource(struct LoadedResource * resource) {
  if(resource->gen != _resource_gen) {
    alias_InlineList_remove_self(&resource->list);
    alias_InlineList_push(&_active_resources, &resource->list);
    resource->gen = _resource_gen;
  }
}

struct LoadedResource * _loaded_resource_by_id(uint32_t id) {
  return _resources.data[id - 1];
}

struct LoadedResource * _load_image(struct Image * img) {
  struct LoadedResource * resource;

  if(img->resource == NULL || img->resource_id != img->resource->id) {
    char path[1024];
    snprintf(path, sizeof(path), "assets/%s", img->path);

    resource = _allocate_resource(ResourceType_Image);
    BackendImage_load(&resource->image, path);
    img->resource = resource;
    img->resource_id = resource->id;
  } else {
    _touch_resource(img->resource);
  }

  return img->resource;
}

// ====================================================================================================================
// Font ===============================================================================================================
enum FontAtlasType {
    FontAtlasType_Bitmap
  , FontAtlasType_SDF
};

struct FontGlyph {
  uint32_t codepoint;

  float advance;

  float plane_left;
  float plane_top;
  float plane_right;
  float plane_bottom;

  float atlas_left;
  float atlas_top;
  float atlas_right;
  float atlas_bottom;
};

struct Font {
  struct Image atlas;
  enum FontAtlasType atlas_type;
  uint32_t num_glyphs;
  const struct FontGlyph * glyphs;
};

const struct FontGlyph * Font_find_glyph(const struct Font * font, const char * text) {
  // TODO handle utf8
  char32_t c = *text;

  // TODO bsearch
  for(uint32_t i = 0; i < font->num_glyphs; i++) {
    if(font->glyphs[i].codepoint == c) {
      return &font->glyphs[i];
    }
  }

  // return space
  return &font->glyphs[0];
}

void Font_draw(struct Font * font, const char * text, float x, float y, float size, float spacing, alias_Color color) {
  #define FONT_DRAW_CHARACTERS_PER_BATCH 128

  struct BackendUIVertex vertexes[4 * FONT_DRAW_CHARACTERS_PER_BATCH];
  uint32_t indexes[6 * FONT_DRAW_CHARACTERS_PER_BATCH];
  uint32_t i = 0;

  struct BackendImage * image = &_load_image(&font->atlas)->image;
  float s_scale = alias_R_ONE / image->width;
  float t_scale = alias_R_ONE / image->height;
  
  while(*text) {
    const struct FontGlyph * glyph = Font_find_glyph(font, text);

    // TODO utf8 advance
    text++;

    #define EMIT(I, V, H) \
    vertexes[i * 4 + I].xy[0] = x + glyph->plane_##H * size; \
    vertexes[i * 4 + I].xy[1] = y + (1.0f - glyph->plane_##V) * size; \
    vertexes[i * 4 + I].rgba[0] = color.r; \
    vertexes[i * 4 + I].rgba[1] = color.g; \
    vertexes[i * 4 + I].rgba[2] = color.b; \
    vertexes[i * 4 + I].rgba[3] = color.a; \
    vertexes[i * 4 + I].st[0] = glyph->atlas_##H * s_scale; \
    vertexes[i * 4 + I].st[1] = 1.0f - (glyph->atlas_##V * t_scale);
    EMIT(0, bottom, left)
    EMIT(1, bottom, right)
    EMIT(2, top, right)
    EMIT(3, top, left)
    #undef EMIT

    indexes[i * 6 + 0] = i * 4 + 0;
    indexes[i * 6 + 1] = i * 4 + 2;
    indexes[i * 6 + 2] = i * 4 + 1;
    indexes[i * 6 + 3] = i * 4 + 2;
    indexes[i * 6 + 4] = i * 4 + 0;
    indexes[i * 6 + 5] = i * 4 + 3;

    x += glyph->advance * size + spacing;
    i++;

    if(i >= FONT_DRAW_CHARACTERS_PER_BATCH) {
      BackendUIVertex_render(image, vertexes, 6 * FONT_DRAW_CHARACTERS_PER_BATCH, indexes);
      i = 0;
    }
  }

  BackendUIVertex_render(image, vertexes, 6 * i, indexes);
}

void Font_measure(struct Font * font, const char * text, float size, float spacing, float * width, float * height) {
  *height = size;
  *width = 0;

  while(*text) {
    const struct FontGlyph * glyph = Font_find_glyph(font, text);

    // TODO utf8 advance
    text++;

    (*width) += glyph->advance * size + spacing;
  }
}

// ====================================================================================================================
// RENDER =============================================================================================================
DEFINE_COMPONENT(Camera)

DEFINE_COMPONENT(DrawRectangle)

DEFINE_COMPONENT(DrawCircle)

DEFINE_COMPONENT(DrawText)

DEFINE_COMPONENT(Sprite)

static void _update_ui(void);

QUERY(_draw_rectangles
  , read(alias_LocalToWorld2D, t)
  , read(DrawRectangle, r)
  , action(
    alias_R
        hw = r->width / 2
      , hh = r->height / 2
      , bl = -hw
      , br =  hw
      , bt = -hh
      , bb =  hh
      ;

    alias_pga2d_Point box[] = {
        alias_pga2d_point(br, bb)
      , alias_pga2d_point(br, bt)
      , alias_pga2d_point(bl, bt)
      , alias_pga2d_point(bl, bb)
      };

    box[0] = alias_pga2d_sandwich_bm(box[0], t->motor);
    box[1] = alias_pga2d_sandwich_bm(box[1], t->motor);
    box[2] = alias_pga2d_sandwich_bm(box[2], t->motor);
    box[3] = alias_pga2d_sandwich_bm(box[3], t->motor);

    struct BackendUIVertex vertexes[] = {
        { .xy[0] = alias_pga2d_point_x(box[0]), .xy[1] = alias_pga2d_point_y(box[0]), .rgba = { r->color.r, r->color.g, r->color.b, r->color.a } }
      , { .xy[0] = alias_pga2d_point_x(box[1]), .xy[1] = alias_pga2d_point_y(box[1]), .rgba = { r->color.r, r->color.g, r->color.b, r->color.a } }
      , { .xy[0] = alias_pga2d_point_x(box[2]), .xy[1] = alias_pga2d_point_y(box[2]), .rgba = { r->color.r, r->color.g, r->color.b, r->color.a } }
      , { .xy[0] = alias_pga2d_point_x(box[3]), .xy[1] = alias_pga2d_point_y(box[3]), .rgba = { r->color.r, r->color.g, r->color.b, r->color.a } }
    };

    uint32_t indexes[] = { 0, 1, 2, 0, 2, 3 };

    BackendUIVertex_render(NULL, vertexes, 6, indexes);
  )
)

void replacement_DrawCircle(float x, float y, float radius, alias_Color color) {
#define NUM_CIRCLE_SEGMENTS 32

  struct BackendUIVertex vertexes[NUM_CIRCLE_SEGMENTS];
  uint32_t indexes[3 * (NUM_CIRCLE_SEGMENTS - 2)];

  for(uint32_t i = 0; i < NUM_CIRCLE_SEGMENTS; i++) {
    alias_R angle = (alias_R)i / NUM_CIRCLE_SEGMENTS * alias_R_PI * 2;
    alias_R s = alias_R_sin(angle);
    alias_R c = alias_R_cos(angle);
    vertexes[i].xy[0] = x + s * radius;
    vertexes[i].xy[1] = y + c * radius;
    vertexes[i].rgba[0] = color.r;
    vertexes[i].rgba[1] = color.g;
    vertexes[i].rgba[2] = color.b;
    vertexes[i].rgba[3] = color.a;
    vertexes[i].st[0] = 0;
    vertexes[i].st[1] = 0;

    if(i >= 2) {
      indexes[(i - 2) * 3 + 0] = 0;
      indexes[(i - 2) * 3 + 1] = i - 1;
      indexes[(i - 2) * 3 + 2] = i - 0;
    }
  }

  BackendUIVertex_render(NULL, vertexes, 3 * (NUM_CIRCLE_SEGMENTS - 2), indexes);

#undef NUM_CIRCLE_SEGMENTS
}

static struct FontGlyph BreeSerif_glyphs[] = {
   { 32,0.20000000000000001,0,0,0,0,0,0,0,0 }
 , { 33,0.27700000000000002,0.049734396671289864,-0.038253814147018048,0.22726560332871013,0.71625381414701805,0.5,178.5,8.5,212.5 }
 , { 34,0.34900000000000003,0.040851595006934839,0.41685159500693481,0.30714840499306523,0.68314840499306528,226.5,12.5,238.5,24.5 }
 , { 35,0.53400000000000003,0.0042988904299583871,-0.019275312066574173,0.51470110957004156,0.62427531206657416,203.5,52.5,226.5,81.5 }
 , { 36,0.53500000000000003,0.03749029126213594,-0.10473231622746189,0.50350970873786416,0.76073231622746185,127.5,216.5,148.5,255.5 }
 , { 37,0.80100000000000005,0.013150485436893244,-0.033562413314840507,0.78984951456310692,0.67656241331484057,173.5,146.5,208.5,178.5 }
 , { 38,0.71399999999999997,0.034128987517337055,-0.055753814147018056,0.69987101248266315,0.69875381414701809,8.5,178.5,38.5,212.5 }
 , { 39,0.20500000000000002,0.035925797503467395,0.41685159500693481,0.16907420249653257,0.68314840499306528,249.5,200.5,255.5,212.5 }
 , { 40,0.33100000000000002,0.049351595006934812,-0.18951941747572815,0.31564840499306518,0.74251941747572825,15.5,213.5,27.5,255.5 }
 , { 41,0.33100000000000002,0.015351595006934809,-0.18751941747572815,0.2816484049930652,0.74451941747572825,40.5,213.5,52.5,255.5 }
 , { 42,0.45200000000000001,0.025777392510402224,0.39387309292649098,0.42522260748959784,0.77112690707350906,188.5,7.5,206.5,24.5 }
 , { 43,0.53500000000000003,0.045085991678224697,0.09358599167822472,0.48891400832177534,0.53741400832177533,146.5,4.5,166.5,24.5 }
 , { 44,0.24099999999999999,0.01913869625520109,-0.17433980582524275,0.2188613037447989,0.13633980582524272,244.5,118.5,253.5,132.5 }
 , { 45,0.32200000000000001,0.016755894590846056,0.22142579750346741,0.30524410540915398,0.35457420249653265,232.5,218.5,245.5,224.5 }
 , { 46,0.22600000000000001,0.024234396671289866,-0.032265603328710125,0.20176560332871013,0.14526560332871011,224.5,216.5,232.5,224.5 }
 , { 47,0.42799999999999999,-0.0089140083217753002,-0.12132801664355065,0.43491400832177535,0.76632801664355066,87.5,215.5,107.5,255.5 }
 , { 48,0.53500000000000003,0.02289459084604718,-0.031562413314840478,0.51110540915395286,0.67856241331484057,208.5,146.5,230.5,178.5 }
 , { 49,0.45400000000000001,0.025181692094313476,-0.021966712898751725,0.44681830790568661,0.66596671289875176,194.5,113.5,213.5,144.5 }
 , { 50,0.51300000000000001,0.047681692094313478,-0.015466712898751732,0.46931830790568663,0.67246671289875182,211.5,81.5,230.5,112.5 }
 , { 51,0.52200000000000002,0.017990291262135919,-0.033062413314840507,0.48400970873786414,0.67706241331484063,230.5,146.5,251.5,178.5 }
 , { 52,0.53600000000000003,0.028894590846047188,-0.014466712898751725,0.51710540915395287,0.67346671289875182,67.5,50.5,89.5,81.5 }
 , { 53,0.50700000000000001,0.015585991678224715,-0.025253814147018012,0.45941400832177531,0.72925381414701806,98.5,178.5,118.5,212.5 }
 , { 54,0.53400000000000003,0.038490291262135955,-0.025062413314840479,0.50450970873786416,0.68506241331484063,0.5,112.5,21.5,144.5 }
 , { 55,0.47500000000000003,0.0059902912621359475,-0.021966712898751725,0.47200970873786419,0.66596671289875176,120.5,50.5,141.5,81.5 }
 , { 56,0.53500000000000003,0.035490291262135931,-0.032062413314840471,0.50150970873786416,0.67806241331484063,21.5,112.5,42.5,144.5 }
 , { 57,0.53500000000000003,0.033990291262135951,-0.032562413314840506,0.50000970873786421,0.67756241331484057,71.5,112.5,92.5,144.5 }
 , { 58,0.22600000000000001,0.024234396671289866,-0.036892510402219109,0.20176560332871013,0.51789251040221929,168.5,25.5,176.5,50.5 }
 , { 59,0.24099999999999999,0.014042995839112338,-0.17346671289875171,0.23595700416088766,0.51446671289875179,239.5,181.5,249.5,212.5 }
 , { 60,0.53500000000000003,0.050085991678224695,0.085490291262135934,0.49391400832177534,0.55150970873786409,106.5,3.5,126.5,24.5 }
 , { 61,0.53500000000000003,0.045585991678224684,0.15066019417475729,0.48941400832177534,0.4613398058252427,206.5,10.5,226.5,24.5 }
 , { 62,0.53500000000000003,0.050085991678224695,0.085490291262135934,0.49391400832177534,0.55150970873786409,126.5,3.5,146.5,24.5 }
 , { 63,0.433,0.018777392510402252,-0.038253814147018048,0.41822260748959783,0.71625381414701805,197.5,178.5,215.5,212.5 }
 , { 64,0.95300000000000007,0.05436338418862692,-0.20523231622746188,0.89763661581137311,0.66023231622746192,154.5,216.5,192.5,255.5 }
 , { 65,0.66200000000000003,-0.012966712898751703,-0.011466712898751711,0.67496671289875176,0.67646671289875182,224.5,224.5,255.5,255.5 }
 , { 66,0.60199999999999998,0.014011789181692108,-0.011466712898751711,0.59098821081830799,0.67646671289875182,0.5,50.5,26.5,81.5 }
 , { 67,0.60199999999999998,0.018607489597780892,-0.034158113730929258,0.57339251040221928,0.69815811373092929,71.5,145.5,96.5,178.5 }
 , { 68,0.66500000000000004,0.018820388349514553,-0.011466712898751711,0.64017961165048543,0.67646671289875182,183.5,81.5,211.5,112.5 }
 , { 69,0.54900000000000004,0.016298890429958406,-0.011466712898751711,0.52670110957004157,0.67646671289875182,160.5,81.5,183.5,112.5 }
 , { 70,0.54300000000000004,0.022798890429958397,-0.011466712898751711,0.53320110957004163,0.67646671289875182,137.5,81.5,160.5,112.5 }
 , { 71,0.64100000000000001,0.028416088765603319,-0.033658113730929265,0.62758391123439672,0.69865811373092923,96.5,145.5,123.5,178.5 }
 , { 72,0.71199999999999997,0.014033287101248293,-0.011466712898751711,0.70196671289875179,0.67646671289875182,69.5,81.5,100.5,112.5 }
 , { 73,0.307,0.011255894590846057,-0.011466712898751711,0.29974410540915397,0.67646671289875182,56.5,81.5,69.5,112.5 }
 , { 74,0.47400000000000003,0.010490291262135942,-0.029062413314840486,0.47650970873786413,0.68106241331484063,92.5,112.5,113.5,144.5 }
 , { 75,0.622,0.011320388349514564,-0.011466712898751711,0.63267961165048547,0.67646671289875182,0.5,81.5,28.5,112.5 }
 , { 76,0.53700000000000003,0.014298890429958404,-0.011466712898751711,0.52470110957004157,0.67646671289875182,163.5,50.5,186.5,81.5 }
 , { 77,0.83399999999999996,0.0064590846047157041,-0.011466712898751711,0.8275409153952844,0.67646671289875182,100.5,81.5,137.5,112.5 }
 , { 78,0.70499999999999996,0.012033287101248282,-0.011466712898751711,0.69996671289875179,0.67646671289875182,213.5,113.5,244.5,144.5 }
 , { 79,0.66400000000000003,0.021320388349514573,-0.034158113730929258,0.64267961165048537,0.69815811373092929,123.5,145.5,151.5,178.5 }
 , { 80,0.58999999999999997,0.018607489597780892,-0.011466712898751711,0.57339251040221928,0.67646671289875182,169.5,113.5,194.5,144.5 }
 , { 81,0.66400000000000003,0.017437586685159524,-0.17723231622746186,0.72756241331484062,0.68823231622746184,192.5,216.5,224.5,255.5 }
 , { 82,0.626,0.013320388349514576,-0.011466712898751711,0.63467961165048548,0.67646671289875182,28.5,81.5,56.5,112.5 }
 , { 83,0.52200000000000002,0.022394590846047179,-0.034158113730929258,0.51060540915395292,0.69815811373092929,151.5,145.5,173.5,178.5 }
 , { 84,0.55300000000000005,-0.00089251040221912173,-0.011466712898751711,0.55389251040221932,0.67646671289875182,230.5,81.5,255.5,112.5 }
 , { 85,0.65900000000000003,0.009724687933425822,-0.029062413314840486,0.65327531206657419,0.68106241331484063,42.5,112.5,71.5,144.5 }
 , { 86,0.65400000000000003,-0.016966712898751724,-0.011466712898751711,0.67096671289875176,0.67646671289875182,89.5,50.5,120.5,81.5 }
 , { 87,0.88500000000000001,-0.012423717059639388,-0.011466712898751711,0.89742371705963941,0.67646671289875182,26.5,50.5,67.5,81.5 }
 , { 88,0.621,-0.011275312066574197,-0.011466712898751711,0.63227531206657417,0.67646671289875182,140.5,113.5,169.5,144.5 }
 , { 89,0.57000000000000006,-0.014583911234396675,-0.011466712898751711,0.58458391123439679,0.67646671289875182,113.5,113.5,140.5,144.5 }
 , { 90,0.53300000000000003,0.018894590846047173,-0.011466712898751711,0.50710540915395297,0.67646671289875182,141.5,50.5,163.5,81.5 }
 , { 91,0.32500000000000001,0.062947295423023589,-0.18092371705963939,0.30705270457697648,0.72892371705963943,65.5,214.5,76.5,255.5 }
 , { 92,0.44,-0.0019140083217753024,-0.12032801664355065,0.44191400832177535,0.76732801664355066,107.5,215.5,127.5,255.5 }
 , { 93,0.32500000000000001,0.017947295423023576,-0.18092371705963939,0.26205270457697638,0.72892371705963943,76.5,214.5,87.5,255.5 }
 , { 94,0.55300000000000005,0.034894590846047173,0.26868169209431347,0.52310540915395287,0.6903183079056866,166.5,5.5,188.5,24.5 }
 , { 95,0.5,-0.016296809986130371,-0.16597850208044385,0.51629680998613037,-0.05502149791955617,196.5,36.5,220.5,41.5 }
 , { 96,0.5,0.14304299583911237,0.54035159500693486,0.36495700416088767,0.80664840499306523,244.5,132.5,254.5,144.5 }
 , { 97,0.56700000000000006,0.023203190013869623,-0.02489251040221914,0.55579680998613035,0.52989251040221919,144.5,25.5,168.5,50.5 }
 , { 98,0.55600000000000005,0.0012031900138696284,-0.027253814147018014,0.53379680998613033,0.72725381414701806,0.5,144.5,24.5,178.5 }
 , { 99,0.47100000000000003,0.01508599167822469,-0.025392510402219137,0.45891400832177531,0.52939251040221913,86.5,25.5,106.5,50.5 }
 , { 100,0.56800000000000006,0.025203190013869631,-0.027253814147018014,0.55779680998613035,0.72725381414701806,215.5,178.5,239.5,212.5 }
 , { 101,0.48399999999999999,0.0210859916782247,-0.02489251040221914,0.46491400832177532,0.52989251040221919,176.5,25.5,196.5,50.5 }
 , { 102,0.37,-0.0018183079056865274,-0.015753814147018021,0.41981830790568658,0.73875381414701802,178.5,178.5,197.5,212.5 }
 , { 103,0.54400000000000004,0.013394590846047175,-0.22525381414701801,0.50160540915395291,0.52925381414701811,156.5,178.5,178.5,212.5 }
 , { 104,0.59999999999999998,0.014511789181692119,-0.020753814147018042,0.59148821081830794,0.73375381414701801,130.5,178.5,156.5,212.5 }
 , { 105,0.29599999999999999,0.018851595006934823,-0.018753814147018034,0.28514840499306526,0.73575381414701801,118.5,178.5,130.5,212.5 }
 , { 106,0.28300000000000003,-0.099435506241331503,-0.22561511789181693,0.23343550624133147,0.72861511789181688,0.5,212.5,15.5,255.5 }
 , { 107,0.55600000000000005,0.020703190013869645,-0.020753814147018042,0.5532968099861304,0.73375381414701801,74.5,178.5,98.5,212.5 }
 , { 108,0.29799999999999999,0.016851595006934821,-0.020753814147018042,0.28314840499306526,0.73375381414701801,62.5,178.5,74.5,212.5 }
 , { 109,0.86599999999999999,0.011863384188626909,-0.01839251040221913,0.85513661581137312,0.53639251040221914,106.5,25.5,144.5,50.5 }
 , { 110,0.59999999999999998,0.012011789181692096,-0.01839251040221913,0.58898821081830799,0.53639251040221914,226.5,56.5,252.5,81.5 }
 , { 111,0.52800000000000002,0.019394590846047173,-0.036988210818307901,0.50760540915395291,0.53998821081830795,0.5,24.5,22.5,50.5 }
 , { 112,0.56800000000000006,0.0097031900138696454,-0.221753814147018,0.54229680998613039,0.53275381414701817,38.5,178.5,62.5,212.5 }
 , { 113,0.55100000000000005,0.024203190013869627,-0.221753814147018,0.55679680998613035,0.53275381414701817,24.5,144.5,48.5,178.5 }
 , { 114,0.46200000000000002,0.0095859916782246885,-0.01839251040221913,0.45341400832177536,0.53639251040221914,66.5,25.5,86.5,50.5 }
 , { 115,0.45100000000000001,0.032777392510402223,-0.02489251040221914,0.43222260748959779,0.52989251040221919,48.5,25.5,66.5,50.5 }
 , { 116,0.377,0.006873092926491008,-0.035371012482662972,0.38412690707350911,0.63037101248266303,186.5,51.5,203.5,81.5 }
 , { 117,0.59099999999999997,0.0065117891816921054,-0.032392510402219139,0.58348821081830793,0.52239251040221912,22.5,25.5,48.5,50.5 }
 , { 118,0.55800000000000005,-0.0094882108183078854,-0.014796809986130368,0.56748821081830791,0.51779680998613042,20.5,0.5,46.5,24.5 }
 , { 119,0.79500000000000004,-0.0019452149791955779,-0.014796809986130368,0.79694521497919546,0.51779680998613042,46.5,0.5,82.5,24.5 }
 , { 120,0.53400000000000003,-0.00129680998613037,-0.014796809986130368,0.53129680998613038,0.51779680998613042,82.5,0.5,106.5,24.5 }
 , { 121,0.57300000000000006,0.0097988904299584085,-0.23275381414701804,0.52020110957004162,0.52175381414701805,48.5,144.5,71.5,178.5 }
 , { 122,0.48299999999999998,0.022085991678224687,-0.014796809986130368,0.46591400832177532,0.51779680998613042,0.5,0.5,20.5,24.5 }
 , { 123,0.34000000000000002,0.029755894590846054,-0.19201941747572812,0.31824410540915399,0.7400194174757283,52.5,213.5,65.5,255.5 }
 , { 124,0.27400000000000002,0.070425797503467405,-0.10323231622746189,0.2035742024965326,0.7622323162274619,148.5,216.5,154.5,255.5 }
 , { 125,0.34000000000000002,0.021755894590846054,-0.19201941747572812,0.31024410540915398,0.7400194174757283,27.5,213.5,40.5,255.5 }
 , { 126,0.59799999999999998,0.032703190013869635,0.20313869625520112,0.56529680998613041,0.40286130374479895,196.5,41.5,220.5,50.5 }
};

struct Font BreeSerif = {
    .atlas      = { .path = "breeserif_soft.png" }
  , .atlas_type = FontAtlasType_Bitmap
  , .num_glyphs = sizeof(BreeSerif_glyphs) / sizeof(BreeSerif_glyphs[0])
  , .glyphs     = BreeSerif_glyphs
};

void replacement_DrawText(const char * text, float x, float y, float size, alias_Color color) {
  Font_draw(&BreeSerif, text, x, y, size, 0, color);
}

void replacement_MeasureTextEx(const char * text, float size, float spacing, float * width, float * height) {
  Font_measure(&BreeSerif, text, size, spacing, width, height);
}

QUERY(_draw_circles
  , read(alias_LocalToWorld2D, transform)
  , read(DrawCircle, c)
  , action(
    replacement_DrawCircle(alias_pga2d_point_x(transform->position), alias_pga2d_point_y(transform->position), c->radius, c->color);
  )
)

QUERY(_draw_text
  , read(alias_LocalToWorld2D, w)
  , read(DrawText, t)
  , action(
    replacement_DrawText(t->text, alias_pga2d_point_x(w->position), alias_pga2d_point_y(w->position), t->size, t->color);
  )
)

QUERY(_draw_sprites
  , read(alias_LocalToWorld2D, t)
  , read(Sprite, s)
  , action(
    struct LoadedResource * res = _load_image(s->image);

    alias_R
        hw = res->image.width / 2
      , hh = res->image.height / 2
      , bl = -hw
      , br =  hw
      , bt = -hh
      , bb =  hh
      ;

    alias_pga2d_Point box[] = {
        alias_pga2d_point(br, bb)
      , alias_pga2d_point(br, bt)
      , alias_pga2d_point(bl, bt)
      , alias_pga2d_point(bl, bb)
      };

    box[0] = alias_pga2d_sandwich_bm(box[0], t->motor);
    box[1] = alias_pga2d_sandwich_bm(box[1], t->motor);
    box[2] = alias_pga2d_sandwich_bm(box[2], t->motor);
    box[3] = alias_pga2d_sandwich_bm(box[3], t->motor);

    struct BackendUIVertex vertexes[] = {
        { .xy = { alias_pga2d_point_x(box[0]), alias_pga2d_point_y(box[0]) }, .rgba = { s->color.r, s->color.g, s->color.b, s->color.a }, .st = { s->s1, s->t1 } }
      , { .xy = { alias_pga2d_point_x(box[1]), alias_pga2d_point_y(box[1]) }, .rgba = { s->color.r, s->color.g, s->color.b, s->color.a }, .st = { s->s1, s->t0 } }
      , { .xy = { alias_pga2d_point_x(box[2]), alias_pga2d_point_y(box[2]) }, .rgba = { s->color.r, s->color.g, s->color.b, s->color.a }, .st = { s->s0, s->t0 } }
      , { .xy = { alias_pga2d_point_x(box[3]), alias_pga2d_point_y(box[3]) }, .rgba = { s->color.r, s->color.g, s->color.b, s->color.a }, .st = { s->s0, s->t1 } }
    };

    uint32_t indexes[6] = {
        0, 1, 2
      , 0, 2, 3
    };

    BackendUIVertex_render(&res->image, vertexes, 6, indexes);
  )
)

QUERY(_update_display
  , read(alias_LocalToWorld2D, transform)
  , read(Camera, camera)
  , pre(
    Backend_begin_rendering(_screen_width, _screen_height);
  )
  , action(
    struct BackendMode2D mode;

    mode.viewport_min = camera->viewport_min;
    mode.viewport_max = camera->viewport_max;
    mode.camera = transform->motor;
    mode.zoom = camera->zoom;
    mode.background = alias_Color_from_rgb_u8(245, 245, 245);
    Backend_begin_2d(mode);

    _draw_sprites();
    _draw_rectangles();
    _draw_circles();
    _draw_text();

    Backend_end_2d();
  )
  , post(
    _update_ui();
    Backend_end_rendering();
  )
)

// ====================================================================================================================
// UI =================================================================================================================

static inline void _text_size(const char * buffer, float size, float max_width, float * out_width, float * out_height) {
  (void)max_width;
  replacement_MeasureTextEx(buffer, size, 0, out_width, out_height);
}

static inline void _text_draw(const char * buffer, float x, float y, float width, float size, alias_Color color) {
  (void)width;
  replacement_DrawText(buffer, x, y, size, color);
}

static inline void _start_ui(void) {
  static alias_ui_Input input;

  if(!_ui_recording) {
    input.screen_size.width = _screen_width;
    input.screen_size.height = _screen_height;
    input.text_size = _text_size;
    input.text_draw = _text_draw;

    alias_ui_begin_frame(_ui, alias_default_MemoryCB(), &input);
    _ui_recording = true;

    alias_ui_begin_stack(_ui);
  }
}

void Engine_ui_image(struct Image * img) {
  _start_ui();
  struct LoadedResource * resource = _load_image(img);
  alias_ui_image(_ui, resource->image.width, resource->image.height, 0, 0, 1, 1, resource->id);
}

void Engine_ui_align_fractions(float x, float y) {
  _start_ui();
  alias_ui_align_fractions(_ui, x, y);
}

void Engine_ui_font_size(alias_R size) {
  _start_ui();
  alias_ui_font_size(_ui, size);
}

void Engine_ui_font_color(alias_Color color) {
  _start_ui();
  alias_ui_font_color(_ui, color);
}

void Engine_ui_text(const char * format, ...) {
  _start_ui();

  va_list ap;
  va_start(ap, format);
  alias_ui_textv(_ui, format, ap);
  va_end(ap);
}

void Engine_ui_vertical(void) {
  _start_ui();
  alias_ui_begin_vertical(_ui);
}

void Engine_ui_horizontal(void) {
  _start_ui();
  alias_ui_begin_horizontal(_ui);
}

void Engine_ui_stack(void) {
  _start_ui();
  alias_ui_begin_stack(_ui);
}

void Engine_ui_end(void) {
  _start_ui();
  alias_ui_end(_ui);
}

static void _update_ui(void) {
  static alias_ui_Output output;

  output.num_groups = 0;
  output.max_groups = UI_NUM_GROUPS;
  output.groups = _ui_groups;
  output.num_indexes = 0;
  output.num_vertexes = 0;
  output.index_sub_buffer = _ui_indexes;
  output.xy_sub_buffer = _ui_vertexes_xy;
  output.rgba_sub_buffer = _ui_vertexes_rgba;
  output.st_sub_buffer = _ui_vertexes_st;

  if(_ui_recording) {
    alias_ui_end(_ui); // end stack
    alias_ui_end_frame(_ui, alias_default_MemoryCB(), &output);
    _ui_recording = false;

    for(uint32_t g = 0; g < output.num_groups; g++) {
      uint32_t length = _ui_groups[g].length;
      if(length == 0) {
        continue;
      }

      struct LoadedResource * material = _loaded_resource_by_id(_ui_groups[g].texture_id);

      BackendUIVertex_render(&material->image, _ui_vertexes_data, _ui_groups[g].length, _ui_indexes_data + _ui_groups[g].index);
    }
  }
}
