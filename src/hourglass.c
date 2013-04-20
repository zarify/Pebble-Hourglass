#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0x26, 0x56, 0xD6, 0x46, 0x07, 0xC0, 0x41, 0xD8, 0x8B, 0x43, 0x7D, 0x37, 0xA3, 0x68, 0x5B, 0xDE }
PBL_APP_INFO(MY_UUID,
             "Hourglasses", "Headcrab",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define GLASS_X 42
#define GLASS_Y 41
#define X_OFF -24
#define Y_OFF -9 // grr x 2

Window window;
Layer hourglass_layer;
Layer hour_layer;
TextLayer date_layer;
RotBmpPairContainer image; // houglass, shows the rough minute
BmpContainer lil_glass; // little hourglass, used to show the hour

// reposition the sand pieces according to the current minute in the hour
// the hour part is managed by the hour_layer little hourglasses
void hourglass_layer_update_callback(Layer *layer, GContext *ctx) {
  PblTm t;
  get_time(&t);

  // get the amount of minutes (increments of 10)
  unsigned int mins = t.tm_min;
  unsigned int pix_up;
  unsigned int pix_down;
  if (mins > 57) { // I'd love to use a switch statement or something here, but the ratio is fudged a bit
    pix_up = 0;
    pix_down = 42;
  } else if (mins > 50) {
    pix_up = 7;
    pix_down = 35;
  } else if (mins > 40) {
    pix_up = 14;
    pix_down = 28;
  } else if (mins > 30) {
    pix_up = 21;
    pix_down = 21;
  } else if (mins > 20) {
    pix_up = 28;
    pix_down = 14;
  } else if (mins > 7) {
    pix_up = 35;
    pix_down = 7;
  } else {
    pix_up = 42;
    pix_down = 0;
  }

  // give the whole hourglass a white background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer->bounds, 0, GCornerNone);
  // each half ignores the top and bottom 3 pixels
  const int16_t half_h = layer->bounds.size.h / 2 - 3;
  // draw the top half of the sand
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0,pix_down,layer->bounds.size.w,half_h+3-pix_down), 0, GCornerNone);
  // graphics_fill_rect(ctx, GRect(0,0,layer->bounds.size.w,half_h+3), 0, GCornerNone); // need to calculate this in future
  // calculate time slice
  // draw the bottom half of the sand
  graphics_fill_rect(ctx, GRect(0,half_h+4+pix_up,layer->bounds.size.w,layer->bounds.size.h-pix_up), 0, GCornerNone);
  // graphics_fill_rect(ctx, GRect(0,half_h+4,layer->bounds.size.w,layer->bounds.size.h), 0, GCornerNone);
}

// create little hourglasses representing the number of hours
void hour_layer_update_callback(Layer *layer, GContext *ctx) {
  PblTm t;
  get_time(&t);
  unsigned int new_x;
  unsigned int new_y;

  for (int i=0;i<t.tm_hour%12;i++) {
    GRect dest = layer_get_frame(&lil_glass.layer.layer);
    new_x = (3+i*18+i*3);
    new_y = 0;
    if (i > 5) {
      new_x = 3+(i-6)*18+(i-6)*3;
      new_y += 17;
    }
    dest.origin.x = new_x;
    dest.origin.y = new_y;
    graphics_draw_bitmap_in_rect(ctx, &lil_glass.bmp, dest);
  }
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Hourglass Face");
  window_stack_push(&window, true /* Animated */);

  window_set_background_color(&window, GColorBlack);

  resource_init_current_app(&APP_RESOURCES);
  rotbmp_pair_init_container(RESOURCE_ID_IMAGE_HOURGLASS_WHITE, RESOURCE_ID_IMAGE_HOURGLASS_BLACK, &image);
  bmp_init_container(RESOURCE_ID_IMAGE_LILGLASS, &lil_glass);
  // set up the hourglass and the image
  layer_init(&hourglass_layer, GRect(GLASS_X,GLASS_Y,60,90));
  hourglass_layer.update_proc = &hourglass_layer_update_callback;
  layer_add_child(&window.layer, &hourglass_layer);
  layer_add_child(&hourglass_layer, &image.layer.layer);
  // reposition the hourglass - rrr, weirdness
  image.layer.layer.frame.origin.x = X_OFF;
  image.layer.layer.frame.origin.y = Y_OFF;
  // set up the little hourglasses
  layer_init(&hour_layer, GRect(3,3,138,31));
  hour_layer.update_proc = &hour_layer_update_callback;
  layer_add_child(&window.layer, &hour_layer);
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
  layer_mark_dirty(&hour_layer);
  // set the date
  static char date_text[] = "Xxxxxxxxx 00";
  string_format_time(date_text, sizeof(date_text), "%B %e", t->tick_time);
  text_layer_set_text(&date_layer, date_text);
}

void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  rotbmp_pair_deinit_container(&image);
  bmp_deinit_container(&lil_glass);
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

