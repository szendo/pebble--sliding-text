#include <pebble.h>
#include "num2words.h"
#include "fontmap.h"
#include "config.h"
  
#define PBL_IF_RECT_ELSE(x, y) x

typedef enum {
  MOVING_IN,
  IN_FRAME,
  PREPARE_TO_MOVE,
  MOVING_OUT
} SlideState;

typedef struct {
  char *text;
  FontType font;
} RowData;

typedef struct {
  Layer *layer;
  SlideState state; // animation state
  char *next_string; // what to say in the next phase of animation

  int left_pos;
  int right_pos;
  int still_pos;

  int movement_delay;
  int delay_count;
} SlidingRow;

typedef struct {
  SlidingRow rows[3];
  int last_hour;
  int last_minute;

  Window *window;
  Animation *animation;

  struct SlidingTextRenderState {
    // double buffered string storage
    char hours[2][32];
    uint8_t next_hours;
    char first_minutes[2][32];
    char second_minutes[2][32];
    uint8_t next_minutes;
  } render_state;

  struct Config {
    uint8_t text_align;
  } config;

} SlidingTextData;

SlidingTextData *s_data;

static void update_proc_sliding_row(Layer *layer, GContext *ctx) {
  RowData *row_data = (RowData*)layer_get_data(layer);
  if (row_data && row_data->text) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    GPoint offset = GPoint(PBL_IF_RECT_ELSE(0, 18), 0);
    if (PBL_IF_RECT_ELSE(s_data->config.text_align == 2, s_data->config.text_align == 0 || s_data->config.text_align == 2)) {
      GRect bounds = layer_get_bounds(layer);
      int16_t text_width = measure_text(row_data->text, row_data->font);
      offset = GPoint((bounds.size.w - text_width) / 2, 0);
    }
    draw_text(ctx, row_data->text, row_data->font, offset);  
  }
}

static void init_sliding_row(SlidingTextData *data, SlidingRow *row, GRect pos, int delay, FontType font_type) {
  row->layer = layer_create_with_data(pos, sizeof(RowData));
  layer_set_update_proc(row->layer, update_proc_sliding_row);
  RowData* row_data = (RowData*)layer_get_data(row->layer);
  row_data->text = NULL;
  row_data->font = font_type;

  row->state = IN_FRAME;
  row->next_string = NULL;

  row->left_pos = -pos.size.w;
  row->right_pos = pos.size.w;
  row->still_pos = pos.origin.x;

  row->movement_delay = delay;
  row->delay_count = 0;

  data->last_hour = -1;
  data->last_minute = -1;
}

static void slide_in_text(SlidingTextData *data, SlidingRow *row, char* new_text) {
  (void) data;

  RowData *row_data = (RowData*)layer_get_data(row->layer);
  if (row_data->text) {
    row->next_string = new_text;
    row->state = PREPARE_TO_MOVE;
  } else {
    row_data->text = new_text;
    GRect frame = layer_get_frame(row->layer);
    frame.origin.x = row->right_pos;
    layer_set_frame(row->layer, frame);
    row->state = MOVING_IN;
  }
}


static bool update_sliding_row(SlidingTextData *data, SlidingRow *row) {

  GRect frame = layer_get_frame(row->layer);
  bool something_changed = true;
  switch (row->state) {
    case PREPARE_TO_MOVE:
      frame.origin.x = row->still_pos;
      row->delay_count++;
      if (row->delay_count > row->movement_delay) {
        row->state = MOVING_OUT;
        row->delay_count = 0;
      }
    break;

    case MOVING_IN: {
      int speed = abs(frame.origin.x - row->still_pos) / 3 + 1;
      frame.origin.x -= speed;
      if (frame.origin.x <= row->still_pos) {
        frame.origin.x = row->still_pos;
        row->state = IN_FRAME;
      }
    }
    break;

    case MOVING_OUT: {
      int speed = abs(frame.origin.x - row->still_pos) / 3 + 1;
      frame.origin.x -= speed;

      if (frame.origin.x <= row->left_pos) {
        frame.origin.x = row->right_pos;
        row->state = MOVING_IN;
        RowData *row_data = (RowData*)layer_get_data(row->layer);
        row_data->text = row->next_string;
        row->next_string = NULL;
      }
    }
    break;

    case IN_FRAME:
    default:
      something_changed = false;
      break;
  }
  if (something_changed) {
    layer_set_frame(row->layer, frame);
  }
  return something_changed;
}

static void animation_update(struct Animation *animation, const AnimationProgress time_normalized) {
  SlidingTextData *data = s_data;

  struct SlidingTextRenderState *rs = &data->render_state;

  time_t now = time(NULL);
  struct tm t = *localtime(&now);

  bool something_changed = false;

  if (data->last_minute != t.tm_min) {
    something_changed = true;

    minute_to_formal_words(t.tm_min, rs->first_minutes[rs->next_minutes], rs->second_minutes[rs->next_minutes]);
    if(data->last_hour != t.tm_hour || t.tm_min <= 20
       || t.tm_min/10 != data->last_minute/10) {
      slide_in_text(data, &data->rows[1], rs->first_minutes[rs->next_minutes]);
    } else {
      // The tens line didn't change, so swap to the correct buffer but don't animate
      RowData *row_data = (RowData*)layer_get_data(data->rows[1].layer);
      row_data->text = rs->first_minutes[rs->next_minutes];
      layer_mark_dirty(data->rows[1].layer);
    }
    slide_in_text(data, &data->rows[2], rs->second_minutes[rs->next_minutes]);
    rs->next_minutes = rs->next_minutes ? 0 : 1;
    data->last_minute = t.tm_min;
  }

  if (data->last_hour != t.tm_hour) {
    hour_to_12h_word(t.tm_hour, rs->hours[rs->next_hours]);
    slide_in_text(data, &data->rows[0], rs->hours[rs->next_hours]);
    rs->next_hours = rs->next_hours ? 0 : 1;
    data->last_hour = t.tm_hour;
  }

  for (size_t i = 0; i < ARRAY_LENGTH(data->rows); ++i) {
    something_changed = update_sliding_row(data, &data->rows[i]) || something_changed;
  }

  if (!something_changed) {
    animation_unschedule(data->animation);
  }
}

static void make_animation() {
  s_data->animation = animation_create();
  animation_set_duration(s_data->animation, ANIMATION_DURATION_INFINITE);
                  // the animation will stop itself
  static const struct AnimationImplementation s_animation_implementation = {
    .update = animation_update,
  };
  animation_set_implementation(s_data->animation, &s_animation_implementation);
  animation_schedule(s_data->animation);
}

static void read_conf_and_force_animations() {
  s_data->config.text_align = config_get_text_align();

  s_data->last_hour = -1;
  s_data->last_minute = -1;
  make_animation();
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  make_animation();
}

static void handle_deinit(void) {
  deinit_fonts();

  config_close();

  tick_timer_service_unsubscribe();
  free(s_data);
}

static void handle_init() {
  init_fonts();
  
  SlidingTextData *data = (SlidingTextData*)malloc(sizeof(SlidingTextData));
  s_data = data;

  data->render_state.next_hours = 0;
  data->render_state.next_minutes = 0;

  data->window = window_create();

  window_set_background_color(data->window, GColorBlack);

  Layer *window_layer = window_get_root_layer(data->window);

  const int16_t width = layer_get_frame(window_layer).size.w;
  init_sliding_row(data, &data->rows[0], GRect(0, 27 + PBL_IF_RECT_ELSE(0, 6), width, 41), 6, BOLD_TYPE);
  layer_add_child(window_layer, data->rows[0].layer);

  init_sliding_row(data, &data->rows[1], GRect(0, 63 + PBL_IF_RECT_ELSE(0, 6), width, 41), 3, LIGHT_TYPE);
  layer_add_child(window_layer, data->rows[1].layer);

  init_sliding_row(data, &data->rows[2], GRect(0, 99 + PBL_IF_RECT_ELSE(0, 6), width, 41), 0, LIGHT_TYPE);
  layer_add_child(window_layer, data->rows[2].layer);

  layer_mark_dirty(window_layer);

  config_register_changed(read_conf_and_force_animations);
  config_open();

  read_conf_and_force_animations();

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

  window_stack_push(data->window, true);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
