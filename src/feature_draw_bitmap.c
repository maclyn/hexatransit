//Main watchface code. The settings code was adapted from the Simplicity2 watchface by
//Matthew Clark.
#include "pebble.h"

static Window *window;
static Layer *layer;

int is_charging = 0;
bool is_connected = true;
int battery_percent = 0;
static GBitmap *numbers[16];
static GBitmap *small_numbers[16];
static GBitmap *charging_icon;
static GBitmap *charging_icon_low;

static AppSync sync;
static uint8_t buffer[256];
enum Settings { setting_power = 1, setting_vibrate, setting_ghost, setting_hourly };
static enum SettingPower { high_power = 0, low_power } power;
static enum SettingVibrate { vibrate_off = 0, vibrate_on } vibrate;
static enum SettingGhost { ghost_off = 0, ghost_on } ghost;
static enum SettingHourly { hourly_off = 0, hourly_on } hourly;
int lowpower_enabled = false;
int vibrate_enabled = true;
int ghost_enabled = true;
int hourly_enabled = true;
int LOW_POWER_KEY = 1;
int VIBRATE_KEY = 2;
int GHOST_KEY = 3;
int HOURLY_KEY = 4;

struct tm* curr_time = NULL;

static void app_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void* context) {
}

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
  if(curr_time == NULL){
    time_t in_time_units;
    time(&in_time_units);
    curr_time = localtime(&in_time_units);
  }
  
  int hours = curr_time->tm_hour;
  int minutes = curr_time->tm_min;
  int seconds = curr_time->tm_sec;
  int month = curr_time->tm_mon;
  int day_of_month = curr_time->tm_mday;
  int day_of_week = curr_time->tm_wday;
  
  if(hourly_enabled && minutes == 0 && seconds == 0){
    vibes_long_pulse();
  }
  
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
  while(hour_x_position > -32 && ghost_enabled){
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
  while(min_x_position > -64 && ghost_enabled){
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
  int seconds_dup = seconds;
  while(seconds_dup > 0){
    graphics_draw_line(ctx, GPoint(x_pos, 97), GPoint(x_pos, 105));
    x_pos += 2;
    seconds_dup--;
  }
  
  //Draw divider between seconds/battery (only if connected)
  if(is_connected) graphics_fill_rect(ctx, (GRect) { .origin = { 123, 100 }, .size = { 3, 3 } }, 0, GCornerNone);
  
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
    if(seconds % 2){
      graphics_draw_bitmap_in_rect(ctx, charging_icon_low,
       (GRect) { .origin = { 128, 99 }, .size = { 14, 5 } });
    } else {
      graphics_draw_bitmap_in_rect(ctx, charging_icon,
       (GRect) { .origin = { 128, 99 }, .size = { 14, 5 } });
    }
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

static void tuple_changed_callback(const uint32_t key, const Tuple* tuple_new, const Tuple* tuple_old, void* context) {
  int value = tuple_new->value->uint8;
  switch (key) {
    case setting_power:
      if ((value >= 0) && (value < 2)){
        lowpower_enabled = value;
        persist_write_bool(LOW_POWER_KEY, value);
      }
      break;
    case setting_vibrate:
      if ((value >= 0) && (value < 2)){
        vibrate_enabled = value;
        persist_write_bool(VIBRATE_KEY, value);
      }
      break;
    case setting_ghost:
      if ((value >= 0) && (value < 2)){
        ghost_enabled = value;
        persist_write_bool(GHOST_KEY, value);
      }
      break;
    case setting_hourly:
      if ((value >= 0) && (value < 2)){
        hourly_enabled = value;
        persist_write_bool(HOURLY_KEY, value);
      }
      break;
  }
}

static void handle_bluetooth(bool connected) {
  is_connected = connected;
  if(vibrate_enabled) vibes_long_pulse();
}

static void handle_battery(BatteryChargeState charge_state) {
  is_charging = charge_state.is_charging;
  battery_percent = charge_state.charge_percent;
}

static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  curr_time = tick_time;
  
  if(!lowpower_enabled || curr_time->tm_sec % 15 == 0){
    layer_mark_dirty(layer);
  }
}

void init(){
  window = window_create();
  window_stack_push(window, true);
  
  if(persist_exists(VIBRATE_KEY)){
    vibrate_enabled = persist_read_bool(VIBRATE_KEY);
  }
  if(persist_exists(LOW_POWER_KEY)){
    lowpower_enabled = persist_read_bool(LOW_POWER_KEY);
  }
  if(persist_exists(GHOST_KEY)){
    ghost_enabled = persist_read_bool(GHOST_KEY);
  }
  if(persist_exists(HOURLY_KEY)){
    hourly_enabled = persist_read_bool(HOURLY_KEY);
  }

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
  charging_icon_low = gbitmap_create_with_resource(RESOURCE_ID_CHARGING_ICON_LOW);
  
  srand(time(NULL));
  
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek());
  handle_battery(battery_state_service_peek());
  
  Tuplet tuples[] = {
    TupletInteger(setting_power, power),
    TupletInteger(setting_vibrate, vibrate),
    TupletInteger(setting_ghost, ghost),
    TupletInteger(setting_hourly, hourly)
  };
  app_message_open(160, 160);
  app_sync_init(&sync, buffer, sizeof(buffer), tuples, ARRAY_LENGTH(tuples), tuple_changed_callback, app_error_callback, NULL);
  layer_mark_dirty(layer);
}

void deinit(){
  app_sync_deinit(&sync);
  
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  
  int i;
  for(i = 0; i < 16; i++){
    gbitmap_destroy(numbers[i]);
    gbitmap_destroy(small_numbers[i]);
  }
  gbitmap_destroy(charging_icon);
  gbitmap_destroy(charging_icon_low);
  
  window_destroy(window);
  layer_destroy(layer);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}