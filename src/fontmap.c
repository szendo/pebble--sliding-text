#include <pebble.h>
#include "fontmap.h"

typedef struct {
  int16_t width;
  int16_t kern_begin;
  int16_t kern_end;
  int16_t load_offset;
  GBitmap *bitmap;
} LetterData;

typedef struct {
  int32_t res_id;
  GBitmap *bitmap;
  int16_t height;
  int16_t space_width;
  LetterData letters[];
} FontData;
 
static FontData s_roboto_bold_params = {
  .res_id = RESOURCE_ID_IMAGE_ROBOTO_BOLD,
  .bitmap = NULL,
  .height = 41,
  .space_width = 10,
  .letters = {
    {23,0,0,0, NULL}, // a
    {24,0,0,0, NULL}, // b
    {22,0,0,0, NULL}, // c
    {24,0,0,0, NULL}, // d
    {23,0,0,0, NULL}, // e
    {16,0,1,0, NULL}, // f
    {24,0,0,0, NULL}, // g
    {24,0,0,0, NULL}, // h
    {11,0,0,0, NULL}, // i
    {13,2,0,2, NULL}, // j
    {23,0,1,1, NULL}, // k
    {11,0,0,0, NULL}, // l
    {36,0,0,0, NULL}, // m
    {24,0,0,0, NULL}, // n
    {24,0,0,0, NULL}, // o
    {24,0,0,0, NULL}, // p
    {24,0,0,0, NULL}, // q
    {15,0,0,0, NULL}, // r
    {22,0,0,0, NULL}, // s
    {14,0,0,0, NULL}, // t
    {24,0,0,0, NULL}, // u
    {21,0,0,0, NULL}, // v
    {31,0,0,0, NULL}, // w
    {22,0,0,0, NULL}, // x
    {21,1,0,1, NULL}, // y
    {21,0,0,0, NULL}, // z
    {10,0,0,0, NULL}  // '
    }
};

static FontData s_roboto_light_params = {
  .res_id = RESOURCE_ID_IMAGE_ROBOTO_LIGHT,
  .bitmap = NULL,
  .height = 41,
  .space_width = 10,
  .letters = {
    {23,0,0,0, NULL}, // a
    {23,0,0,0, NULL}, // b
    {22,0,0,0, NULL}, // c
    {23,0,0,0, NULL}, // d
    {22,0,0,0, NULL}, // e
    {15,0,1,0, NULL}, // f
    {23,0,0,0, NULL}, // g
    {23,0,0,0, NULL}, // h
    { 9,0,0,2, NULL}, // i
    {12,2,0,0, NULL}, // j
    {21,0,0,0, NULL}, // k
    { 9,0,0,0, NULL}, // l
    {37,0,0,0, NULL}, // m
    {23,0,0,0, NULL}, // n
    {24,0,0,0, NULL}, // o
    {23,0,0,0, NULL}, // p
    {23,0,0,0, NULL}, // q
    {14,0,0,0, NULL}, // r
    {21,0,0,0, NULL}, // s
    {14,0,0,0, NULL}, // t
    {23,0,0,0, NULL}, // u
    {20,0,0,0, NULL}, // v
    {32,0,0,0, NULL}, // w
    {20,0,0,0, NULL}, // x
    {20,0,0,0, NULL}, // y
    {20,0,0,0, NULL}, // z
    { 7,0,0,0, NULL}  // '
    }
};

static void init_font(FontData *font_data) {
  font_data->bitmap = gbitmap_create_with_resource(font_data->res_id);
  int16_t offset = 0;
  for (int i = 0; i < 27; i++) {
    LetterData *letter = &(font_data->letters[i]);
    letter->bitmap = gbitmap_create_as_sub_bitmap(font_data->bitmap, GRect(offset - letter->kern_begin, 0, letter->width, font_data->height));
    offset += letter->width -letter->kern_end - letter->kern_begin + letter->load_offset;
  }
}

static void deinit_font(FontData *font_data) {
  if (font_data->bitmap != NULL) {
    for(int i = 0; i < 27; i++) {
      LetterData *letter = &(font_data->letters[i]);
      if (letter->bitmap != NULL) {
        gbitmap_destroy(letter->bitmap);
        letter->bitmap = NULL;
      }
    }
    gbitmap_destroy(font_data->bitmap);
    font_data->bitmap = NULL;
  }
}

void init_fonts() {
  init_font(&s_roboto_bold_params);
  init_font(&s_roboto_light_params);
}

void deinit_fonts() {
  deinit_font(&s_roboto_bold_params);
  deinit_font(&s_roboto_light_params);
}

void draw_text(GContext *ctx, char* text, FontType font_type) {
  FontData *font_data = ((font_type == BOLD_TYPE) ? (&s_roboto_bold_params) : (&s_roboto_light_params));
  int16_t offset = 0;
  for (char *t = text; *t != '\0'; t++) {
    if (*t >= 'a' && *t <= 'z') {
      LetterData *letter = &(font_data->letters[*t - 'a']);
      graphics_draw_bitmap_in_rect(ctx, letter->bitmap, GRect(offset - letter->kern_begin, 0, letter->width, font_data->height));
      offset += letter->width - letter->kern_end;
    } else if (*t == '\'') {
      LetterData *letter = &(font_data->letters[26]);
      graphics_draw_bitmap_in_rect(ctx, letter->bitmap, GRect(offset - letter->kern_begin, 0, letter->width, font_data->height));
      offset += letter->width - letter->kern_end;
    } else {
      offset += font_data->space_width;
    }
  }
}
