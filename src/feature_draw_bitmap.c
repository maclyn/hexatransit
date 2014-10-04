#include "pebble.h"

static Window *window;
static Layer *layer;
static GBitmap *numbers[16];
static GBitmap *small_numbers[16];
static GBitmap *charging_icon;

int is_charging;
int battery_percent = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;
int month = 0;
int day_of_month = 0;
int day_of_week = 0;

static void smear_location(GContext* ctx, int x, int y, int pixels){
  graphics_context_set_stroke_color(ctx, GColorWhite);
  while(pixels > 0){
    int x_smear = rand() % 30;
    int y_smear = rand() % 45;
    graphics_draw_pixel(ctx, GPoint(x + x_smear, y + y_smear));
    pixels--;
  }
  graphics_context_set_stroke_color(ctx, GColorBlack);
}

static void draw_hex(int value, GContext* ctx, int x, int y){
  graphics_draw_bitmap_in_rect(ctx, numbers[value],
     (GRect) { .origin = { x, y }, .size = { 30, 45 } });
}

static void draw_small_hex(int value, GContext* ctx, int x, int y){
  graphics_draw_bitmap_in_rect(ctx, small_numbers[value],
     (GRect) { .origin = { x, y }, .size = { 12, 18 } });
}

static void layer_update_callback(Layer *me, GContext* ctx) {
  //Draw the hours in hex
  int hour_value = hours;
  if(hour_value > 11){
    hour_value -= 12;
  }
  int hour_x_position = 2 + (int)((((float)hours) / (23.0f)) * 110.0f);
  draw_hex(hour_value, ctx, hour_x_position, 2); 
  
  //If there's room to left, "smear" the hex value to the left by randomly painting white
  //pixels over it
  int hour_smear_count = 0;
  hour_x_position -= 36;
  while(hour_x_position > -32){
    hour_smear_count++;
    draw_hex(hour_value, ctx, hour_x_position, 2); 
    //"Smear" the location by drawing random white pixels 20 x count times over the image
    smear_location(ctx, hour_x_position, 2, 300 * hour_smear_count);
    hour_x_position -= 32;
  }
  
  //Draw the minutes in hex
  int first_hex_digit = minutes / 16;
  int second_hex_digit = minutes % 16;
  int min_x_position = 2 + (int)((((float)minutes) / (60.0f)) * 78.0f);
  draw_hex(first_hex_digit, ctx, min_x_position, 49); 
  draw_hex(second_hex_digit, ctx, min_x_position + 32, 49); 
  
  //If there's room to left, "smear" the hex value to the left by randomly painting white
  //pixels over it
  int min_smear_count = 0;
  min_x_position -= 68;
  while(min_x_position > -64){
    min_smear_count++;
    draw_hex(first_hex_digit, ctx, min_x_position, 49); 
    draw_hex(second_hex_digit, ctx, min_x_position + 32, 49); 
    //"Smear" the location by drawing random white pixels 20 x count times over the image
    smear_location(ctx, min_x_position, 49, 300 * min_smear_count);
    smear_location(ctx, min_x_position + 32, 49, 300 * min_smear_count);
    min_x_position -= 64;
  }
  
  //Draw the seconds with lines
  int x_pos = 2;
  while(seconds > 0){
    graphics_draw_line(ctx, GPoint(x_pos, 97), GPoint(x_pos, 105));
    x_pos += 2;
    seconds--;
  }
  
  //Draw divider between seconds/battery
  graphics_fill_rect(ctx, (GRect) { .origin = { 123, 100 }, .size = { 3, 3 } }, 0, GCornerNone);
  
  //Draw battery with lines
  if(!is_charging){
    int battery_lines = (int)((((float)battery_percent) / (100.0f)) * 7.0f);
    int battery_x_pos = 128;
    while(battery_lines > 0){
      graphics_draw_line(ctx, GPoint(battery_x_pos, 99), GPoint(battery_x_pos, 103));
      battery_x_pos += 2;
      battery_lines--;
    }
  } else { //Draw charging image
    graphics_draw_bitmap_in_rect(ctx, charging_icon,
     (GRect) { .origin = { 128, 99 }, .size = { 14, 5 } });
  }
  
  //Draw day of week (0-7) (@ y = 107)
  int dow_x_position = 2 + (int)((((float)day_of_week) / (6.0f)) * 128.0f);
  draw_small_hex(day_of_week+1, ctx, dow_x_position, 107); 
  
  //Draw day of month (0-31) (@ y = 127)
  int dom_hex_1_val = day_of_month / 16;
  int dom_hex_2_val = day_of_month % 16;
  int dom_x_position = 2 + (int)((((float)day_of_month-1) / (30.0f)) * 114.0f);
  draw_small_hex(dom_hex_1_val, ctx, dom_x_position, 127); 
  draw_small_hex(dom_hex_2_val, ctx, dom_x_position + 14, 127); 
  
  //Draw month (0-11) (@ y = 147)
  int m_x_position = 2 + (int)((((float)month) / (11.0f)) * 128.0f);
  draw_small_hex(month+1, ctx, m_x_position, 147); 
}

static void handle_battery(BatteryChargeState charge_state) {
  is_charging = charge_state.is_charging;
  battery_percent = charge_state.charge_percent;
}

static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  handle_battery(battery_state_service_peek());
  
  seconds = tick_time->tm_sec;
  minutes = tick_time->tm_min;
  hours = tick_time->tm_hour;
  month = tick_time->tm_mon;
  day_of_month = tick_time->tm_mday;
  day_of_week = tick_time->tm_wday;
  
  layer_mark_dirty(layer);
}

void init(){
  window = window_create();
  window_stack_push(window, true);

  // Init the layer for display the image
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  layer = layer_create(bounds);
  layer_set_update_proc(layer, layer_update_callback);
  
  layer_add_child(window_layer, layer);
  
  numbers[0] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_0);
  numbers[1] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_1);
  numbers[2] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_2);
  numbers[3] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_3);
  numbers[4] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_4);
  numbers[5] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_5);
  numbers[6] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_6);
  numbers[7] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_7);
  numbers[8] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_8);
  numbers[9] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_9);
  numbers[10] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_A);
  numbers[11] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_B);
  numbers[12] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_C);
  numbers[13] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_D);
  numbers[14] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_E);
  numbers[15] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_F);
  small_numbers[0] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_0);
  small_numbers[1] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_1);
  small_numbers[2] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_2);
  small_numbers[3] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_3);
  small_numbers[4] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_4);
  small_numbers[5] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_5);
  small_numbers[6] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_6);
  small_numbers[7] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_7);
  small_numbers[8] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_8);
  small_numbers[9] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_9);
  small_numbers[10] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_A);
  small_numbers[11] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_B);
  small_numbers[12] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_C);
  small_numbers[13] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_D);
  small_numbers[14] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_E);
  small_numbers[15] = gbitmap_create_with_resource(RESOURCE_ID_SMALL_NUMBER_F);
  charging_icon = gbitmap_create_with_resource(RESOURCE_ID_CHARGING_ICON);
  
  srand(time(NULL));
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
  battery_state_service_subscribe(&handle_battery);
}

void deinit(){
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  
  int i;
  for(i = 0; i < 16; i++){
    gbitmap_destroy(numbers[i]);
    gbitmap_destroy(small_numbers[i]);
  }
  gbitmap_destroy(charging_icon);
  
  window_destroy(window);
  layer_destroy(layer);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}