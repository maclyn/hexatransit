#include "pebble.h"

static Window *window;
static Layer *layer;
static TextLayer *date_layer;
static GBitmap *backdrop;
static GBitmap *slider;
static GBitmap *numbers[16];

int is_charging;
int battery_percent = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;
char date_string[50] = "DOW, Month DOM";

static void smear_location(GContext* ctx, int x, int y, int pixels){
  graphics_context_set_stroke_color(ctx, GColorWhite);
  while(pixels > 0){
    int x_smear = rand() % 20;
    int y_smear = rand() % 28;
    graphics_draw_pixel(ctx, GPoint(x + x_smear, y + y_smear));
    pixels--;
  }
  graphics_context_set_stroke_color(ctx, GColorBlack);
}

static void draw_hex(int value, GContext* ctx, int x, int y){
  graphics_draw_bitmap_in_rect(ctx, numbers[value],
     (GRect) { .origin = { x, y }, .size = { 20, 28 } });
}

static void layer_update_callback(Layer *me, GContext* ctx) {
  //Draw the backdrop
  graphics_draw_bitmap_in_rect(ctx, backdrop, (GRect) { .origin = { 0, 0 }, .size = { 144, 168 } });
  
  //Draw the hours in hex
  int hour_value = hours;
  if(hour_value > 11){
    hour_value -= 12;
  }
  int hour_x_position = 1 + (int)((((float)hours) / (23.0f)) * 122.0f);
  draw_hex(hour_value, ctx, hour_x_position, 13); 
  
  //If there's room to left, "smear" the hex value to the left by randomly painting white
  //pixels over it
  int hour_smear_count = 0;
  hour_x_position -= 23;
  while(hour_x_position > -23){
    hour_smear_count++;
    draw_hex(hour_value, ctx, hour_x_position, 13); 
    //"Smear" the location by drawing random white pixels 20 x count times over the image
    smear_location(ctx, hour_x_position, 13, 100 * hour_smear_count);
    hour_x_position -= 23;
  }
  
  //Draw the minutes in hex
  int first_hex_digit = minutes / 16;
  int second_hex_digit = minutes % 16;
  int min_x_position = 1 + (int)((((float)minutes) / (60.0f)) * 100.0f);
  draw_hex(first_hex_digit, ctx, min_x_position, 81); 
  draw_hex(second_hex_digit, ctx, min_x_position + 23, 81); 
  
  //If there's room to left, "smear" the hex value to the left by randomly painting white
  //pixels over it
  int min_smear_count = 0;
  min_x_position -= 46;
  while(min_x_position > -46){
    min_smear_count++;
    draw_hex(first_hex_digit, ctx, min_x_position, 81); 
    draw_hex(second_hex_digit, ctx, min_x_position + 23, 81); 
    //"Smear" the location by drawing random white pixels 20 x count times over the image
    smear_location(ctx, min_x_position, 81, 100 * min_smear_count);
    smear_location(ctx, min_x_position + 23, 81, 100 * min_smear_count);
    min_x_position -= 46;
  }
  
  //Draw the seconds with lines
  int x_pos = 12;
  while(seconds > 0){
    graphics_draw_line(ctx, GPoint(x_pos, 116), GPoint(x_pos, 123));
    x_pos += 2;
    seconds--;
  }
  
  //Draw the battery status
  int y_position = 47 + (int)((((float)battery_percent) / (100.0f)) * 22.0f);
  graphics_draw_bitmap_in_rect(ctx, slider, (GRect) { .origin = { 125, y_position}, .size = {18, 6}});
}

static void handle_battery(BatteryChargeState charge_state) {
  is_charging = charge_state.is_charging;
  battery_percent = charge_state.charge_percent;
}

static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  strftime(date_string, sizeof(date_string), "%A, %B %d", tick_time);
  text_layer_set_text(date_layer, date_string);
  handle_battery(battery_state_service_peek());
  
  seconds = tick_time->tm_sec;
  minutes = tick_time->tm_min;
  hours = tick_time->tm_hour;
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
  
  date_layer = text_layer_create(GRect(2, 128, 140, 40));
  text_layer_set_text_color(date_layer, GColorBlack);
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(date_layer, GTextAlignmentLeft);
  text_layer_set_overflow_mode(date_layer, GTextOverflowModeWordWrap);
  
  layer_add_child(window_layer, layer);
  layer_add_child(window_layer, text_layer_get_layer(date_layer));
  
  backdrop = gbitmap_create_with_resource(RESOURCE_ID_BACKDROP);
  slider = gbitmap_create_with_resource(RESOURCE_ID_SWITCH);
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
  
  srand(time(NULL));
  
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
  battery_state_service_subscribe(&handle_battery);
}

void deinit(){
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  
  gbitmap_destroy(backdrop);
  gbitmap_destroy(slider);
  int i;
  for(i = 0; i < 15; i++){
    gbitmap_destroy(numbers[i]);
  }
  window_destroy(window);
  layer_destroy(layer);
  text_layer_destroy(date_layer);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
