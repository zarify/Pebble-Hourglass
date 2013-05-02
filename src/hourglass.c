#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0x26, 0x56, 0xD6, 0x46, 0x07, 0xC0, 0x41, 0xD8, 0x8B, 0x43, 0x7D, 0x37, 0xA3, 0x68, 0x5B, 0xDE }
PBL_APP_INFO(MY_UUID,
             "Hourglasses", "Headcrab",
             1, 1, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define GLASS_X 37
#define GLASS_Y 10
#define X_OFF -36 // need some reshuffling to account for alignment weirdness. grr.
#define Y_OFF -9 // grr x 2

Window window;
Layer hourglass_layer;
TextLayer hour_layer;
TextLayer date_layer;
RotBmpPairContainer image; // houglass, shows the rough minute
BmpContainer lil_glass; // little hourglass, used to show the hour

// reposition the sand pieces according to the current minute in the hour
// the hour part is managed by the hour_layer little hourglasses
void hourglass_layer_update_callback(Layer *layer, GContext *ctx) {
  PblTm t;
  get_time(&t);

  unsigned int mins = t.tm_min;

  // give the whole hourglass a white background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer->bounds, 0, GCornerNone);
  // draw the top half of the sand
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0,62-(60-mins),layer->bounds.size.w,60-mins), 0, GCornerNone);
  // draw the bottom half of the sand
  graphics_fill_rect(ctx, GRect(0,(63+60-mins),layer->bounds.size.w,mins), 0, GCornerNone);
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Hourglass Face");
  window_stack_push(&window, true /* Animated */);

  window_set_background_color(&window, GColorBlack);

  resource_init_current_app(&APP_RESOURCES);
  rotbmp_pair_init_container(RESOURCE_ID_IMAGE_HOURGLASS_WHITE, RESOURCE_ID_IMAGE_HOURGLASS_BLACK, &image);
  // set up the hourglass and the image
  layer_init(&hourglass_layer, GRect(GLASS_X,GLASS_Y,70,124));
  hourglass_layer.update_proc = &hourglass_layer_update_callback;
  layer_add_child(&window.layer, &hourglass_layer);
  layer_add_child(&hourglass_layer, &image.layer.layer);
  // reposition the hourglass - rrr, weirdness
  image.layer.layer.frame.origin.x = X_OFF;
  image.layer.layer.frame.origin.y = Y_OFF;
  // set up the hour layer
  text_layer_init(&hour_layer, window.layer.frame);
  text_layer_set_text_color(&hour_layer, GColorWhite);
  text_layer_set_background_color(&hour_layer, GColorClear);
  layer_set_frame(&hour_layer.layer, GRect(2,62,30,40));
  text_layer_set_font(&hour_layer, fonts_get_system_font(FONT_KEY_GOTHAM_30_BLACK));
  text_layer_set_text_alignment(&hour_layer, GTextAlignmentLeft);
  layer_add_child(&window.layer, &hour_layer.layer);
  // set up the date layer
  text_layer_init(&date_layer, window.layer.frame);
  text_layer_set_text_color(&date_layer, GColorWhite);
  text_layer_set_background_color(&date_layer, GColorClear);
  layer_set_frame(&date_layer.layer, GRect(2,130,144-8,168-130));
  text_layer_set_font(&date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&date_layer, GTextAlignmentCenter);
  layer_add_child(&window.layer, &date_layer.layer);
}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)ctx;
  (void)t;
  layer_mark_dirty(&hourglass_layer);
  // set the date
  static char date_text[] = "Xxxxxxxxx 00";
  string_format_time(date_text, sizeof(date_text), "%B %e", t->tick_time);
  text_layer_set_text(&date_layer, date_text);
  // set the hour
  static char hour_text[] = "XX";
  string_format_time(hour_text, sizeof(hour_text), "%l", t->tick_time);
  text_layer_set_text(&hour_layer, hour_text);
}

void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  rotbmp_pair_deinit_container(&image);
  //bmp_deinit_container(&lil_glass);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    }
  };
  app_event_loop(params, &handlers);
}

