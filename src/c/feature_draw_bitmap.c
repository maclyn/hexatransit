// Minimally updated version of a smartwatch I built while I was a student
// Some questionable stuff here! Only what's necessary to ACTUALLY get this
// building and working again in 2026 has been changed with modifications
// to stretch to Pebble Time 2. Be warned.

#include "common.h"
#include "pebble.h"

// VSCode include hacks

#ifndef PBL_DISPLAY_WIDTH
#define PBL_DISPLAY_HEIGHT 228
#define PBL_DISPLAY_WIDTH 200
#define PLAT PlatformTypeEmery
#endif

#ifndef RESOURCE_ID_SMALL_NUMBER_F
#include "resource_ids.auto.h"
#endif

#define BIG_DIGIT_WIDTH_PX 30
#define BIG_DIGIT_HEIGHT_PX 45
#define SMALL_DIGIT_WIDTH_PX 12
#define SMALL_DIGIT_HEIGHT_PX 18

// Seconds - Connected - Battery
#define SECONDS_BATT_CONN_ROW_TOTAL_HEIGHT_PX 8
#define SECONDS_INDICATOR_HEIGHT_PX SECONDS_BATT_CONN_ROW_TOTAL_HEIGHT_PX
#define SECONDS_INDICATOR_WIDTH_PX 1
#define SECONDS_INDICATOR_INTERNAL_PADDING_W_PX 1
#define CONNECTED_ICON_W_H_PX 3
#define CONN_ICON_OFFSET_INSIDE_ROW_PX                                         \
  ((SECONDS_BATT_CONN_ROW_TOTAL_HEIGHT_PX - CONNECTED_ICON_W_H_PX) / 2)
#define BATTERY_BAR_WIDTH_PX 1
#define BATTERY_BAR_HEIGHT_PX 4
#define BATTERY_BAR_INTERNAL_PADDING_W_PX 1
#define CHARGING_ICON_WIDTH_PX 14
#define CHARGING_ICON_HEIGHT_PX 5
#define BATT_OFFSET_INSIDE_ROW_PX                                              \
  ((SECONDS_BATT_CONN_ROW_TOTAL_HEIGHT_PX - BATTERY_BAR_HEIGHT_PX) / 2)
#define SECONDS_BATT_CONN_ROW_TOTAL_WIDTH_PX                                   \
  ((60 * SECONDS_INDICATOR_WIDTH_PX) +                                         \
   (59 * SECONDS_INDICATOR_INTERNAL_PADDING_W_PX) + INTERNAL_ITEM_PADDING_PX + \
   CONNECTED_ICON_W_H_PX + INTERNAL_ITEM_PADDING_PX + CHARGING_ICON_WIDTH_PX)
#define SECONDS_BATT_CONN_ROW_LEFT_OFFSET_PX                                   \
  ((PBL_DISPLAY_WIDTH - SECONDS_BATT_CONN_ROW_TOTAL_WIDTH_PX) / 2)

#define ROWS_OF_CONTENT 6 // Hours, minutes, seconds/battery, 3 date comps.
#define CONTENT_TOTAL_HEIGHT_PX                                                \
  ((BIG_DIGIT_HEIGHT_PX * 2) + (SMALL_DIGIT_HEIGHT_PX * 3) +                   \
   SECONDS_INDICATOR_HEIGHT_PX) // (45 * 2) + (18 * 3) + 8 = 152
#define EXTERNAL_PADDING_TOTAL_PX (PBL_DISPLAY_HEIGHT - CONTENT_TOTAL_HEIGHT_PX)
#define INTERNAL_ITEM_PADDING_PX 2
#define EXTERNAL_ITEM_HORIZONTAL_PADDING_PX 2
#define EXTERNAL_ITEM_VERTICAL_PADDING_PX                                      \
  (EXTERNAL_PADDING_TOTAL_PX / (ROWS_OF_CONTENT + 2))
// EXTERNAL_ITEM_PADDING_PX resolves to 9.5 on PT2, integer rounding down to 9
// This "throws out" (.5 * 8) pixels that we have to account for, so we pad in
// an extra 2 pixels on top
// On everything else, it resolves to 2
#define EXTRA_VERTICAL_PADDING_PX                                              \
  (PBL_PLATFORM_TYPE_CURRENT == PlatformTypeEmery ? 2 : 0)
#define HORIZONTAL_TRACK_WIDTH_PX                                              \
  (PBL_DISPLAY_WIDTH - (2 * EXTERNAL_ITEM_HORIZONTAL_PADDING_PX))
#define SINGLE_WIDE_BIG_DIGIT_MOVEMENT_WIDTH_PX                                \
  (HORIZONTAL_TRACK_WIDTH_PX - BIG_DIGIT_WIDTH_PX)
#define DOUBLE_WIDE_BIG_DIGIT_MOVEMENT_WIDTH_PX                                \
  (HORIZONTAL_TRACK_WIDTH_PX - (BIG_DIGIT_WIDTH_PX * 2) -                      \
   INTERNAL_ITEM_PADDING_PX)
#define SINGLE_WIDE_SMALL_DIGIT_MOVEMENT_WIDTH_PX                              \
  (HORIZONTAL_TRACK_WIDTH_PX - SMALL_DIGIT_WIDTH_PX)
#define DOUBLE_WIDE_SMALL_DIGIT_MOVEMENT_WIDTH_PX                              \
  (HORIZONTAL_TRACK_WIDTH_PX - (SMALL_DIGIT_WIDTH_PX * 2) -                    \
   INTERNAL_ITEM_PADDING_PX)

#define HEX_DIGIT_COUNT 16

#define SETTINGS_KEY 1

const int BIG_NUMBER_RESOURCE_IDS[] = {
    RESOURCE_ID_NUMBER_0, RESOURCE_ID_NUMBER_1, RESOURCE_ID_NUMBER_2,
    RESOURCE_ID_NUMBER_3, RESOURCE_ID_NUMBER_4, RESOURCE_ID_NUMBER_5,
    RESOURCE_ID_NUMBER_6, RESOURCE_ID_NUMBER_7, RESOURCE_ID_NUMBER_8,
    RESOURCE_ID_NUMBER_9, RESOURCE_ID_NUMBER_A, RESOURCE_ID_NUMBER_B,
    RESOURCE_ID_NUMBER_C, RESOURCE_ID_NUMBER_D, RESOURCE_ID_NUMBER_E,
    RESOURCE_ID_NUMBER_F};
const int SMALL_NUMBER_RESOURCE_IDS[] = {
    RESOURCE_ID_SMALL_NUMBER_0, RESOURCE_ID_SMALL_NUMBER_1,
    RESOURCE_ID_SMALL_NUMBER_2, RESOURCE_ID_SMALL_NUMBER_3,
    RESOURCE_ID_SMALL_NUMBER_4, RESOURCE_ID_SMALL_NUMBER_5,
    RESOURCE_ID_SMALL_NUMBER_6, RESOURCE_ID_SMALL_NUMBER_7,
    RESOURCE_ID_SMALL_NUMBER_8, RESOURCE_ID_SMALL_NUMBER_9,
    RESOURCE_ID_SMALL_NUMBER_A, RESOURCE_ID_SMALL_NUMBER_B,
    RESOURCE_ID_SMALL_NUMBER_C, RESOURCE_ID_SMALL_NUMBER_D,
    RESOURCE_ID_SMALL_NUMBER_E, RESOURCE_ID_SMALL_NUMBER_F};

typedef struct ClaySettings {
  bool PowerMode;
  bool GhostTime;
  bool GhostDate;
  bool HourlyVibrate;
  bool DisconnectVibrate;
} ClaySettings;

static Window *window;
static Layer *layer;
static Layer *window_layer;
ClaySettings settings;
static GRect bounds;

static GBitmap *big_numerals_bmps[HEX_DIGIT_COUNT];
static GBitmap *small_numerals_bmps[HEX_DIGIT_COUNT];
static GBitmap *fades_htl_sz_amnt_bmps[2][2];
static GBitmap *charging_icon_bmp;
static GBitmap *charging_icon_low_bmp;

long time_running = 0L;
int is_charging = 0;
bool is_connected = true;
int battery_percent = 0;

static void draw_hex(int value, bool is_small, GContext *ctx, int x, int y,
                     int w, int h) {
  const GBitmap *bmp =
      is_small ? small_numerals_bmps[value] : big_numerals_bmps[value];
  graphics_draw_bitmap_in_rect(ctx, bmp,
                               (GRect){.origin = {x, y}, .size = {w, h}});
}

static void draw_hex_small(GContext *ctx, int value, int x, int y) {
  draw_hex(value, true, ctx, x, y, SMALL_DIGIT_WIDTH_PX, SMALL_DIGIT_HEIGHT_PX);
}

static void draw_hex_big(GContext *ctx, int value, int x, int y) {
  draw_hex(value, false, ctx, x, y, BIG_DIGIT_WIDTH_PX, BIG_DIGIT_HEIGHT_PX);
}

static void smear_left_from_location(int x, int y, bool is_double_wide,
                                     bool is_small) {

  // TODO
}

static void layer_update_callback(Layer *me, GContext *ctx) {
  VERBOSE_LOG("layer_update_callback()");

  time_t in_time_units;
  time(&in_time_units);
  struct tm *curr_time = localtime(&in_time_units);

  int hours = curr_time->tm_hour;
  int minutes = curr_time->tm_min;
  int seconds = curr_time->tm_sec;
  int month = curr_time->tm_mon;
  int day_of_month = curr_time->tm_mday;
  int day_of_week = curr_time->tm_wday;

  VERBOSE_LOG("Time is: %d:%d:%d", hours, minutes, seconds);

  if (settings.HourlyVibrate && minutes == 0 && seconds == 0) {
    vibes_long_pulse();
  }

  // Draw the hours in hex
  // Always renders in 12 hour time
  int hour_value = hours;
  if (hour_value > 11) {
    hour_value -= 12;
  }

  int y_offset = EXTERNAL_ITEM_VERTICAL_PADDING_PX + EXTRA_VERTICAL_PADDING_PX;

  float hour_pct_across_screen = (float)hours / 23.0F;
  int hour_x_position =
      EXTERNAL_ITEM_HORIZONTAL_PADDING_PX +
      (hour_pct_across_screen * SINGLE_WIDE_BIG_DIGIT_MOVEMENT_WIDTH_PX);
  draw_hex_big(ctx, hour_value, hour_x_position, y_offset);
  // smear_location(hour_value, ctx, hour_x_position, edge_padding,
  //                BIG_DIGIT_WIDTH_PX, BIG_DIGIT_HEIGHT_PX, 200, edge_padding,
  //                false);
  y_offset += BIG_DIGIT_HEIGHT_PX + EXTERNAL_ITEM_VERTICAL_PADDING_PX;

  // Draw the minutes in hex
  int first_hex_digit = minutes / HEX_DIGIT_COUNT;
  int second_hex_digit = minutes % HEX_DIGIT_COUNT;
  float min_pct_across_screen = (float)minutes / 60.0F;
  int min_x_position =
      EXTERNAL_ITEM_HORIZONTAL_PADDING_PX +
      (min_pct_across_screen * DOUBLE_WIDE_BIG_DIGIT_MOVEMENT_WIDTH_PX);
  draw_hex_big(ctx, first_hex_digit, min_x_position, y_offset);
  draw_hex_big(ctx, second_hex_digit,
               min_x_position + BIG_DIGIT_WIDTH_PX + INTERNAL_ITEM_PADDING_PX,
               y_offset);
  // smear_location_two_wide(first_hex_digit, second_hex_digit, ctx,
  //                         min_x_position, 49, BIG_DIGIT_WIDTH_PX,
  //                         BIG_DIGIT_HEIGHT_PX, 2, 200, false);
  y_offset += BIG_DIGIT_HEIGHT_PX + EXTERNAL_ITEM_VERTICAL_PADDING_PX;

  // Draw the seconds with lines
  int x_pos = SECONDS_BATT_CONN_ROW_LEFT_OFFSET_PX;
  int seconds_dup = seconds;
  while (seconds_dup > 0) {
    graphics_draw_line(ctx, GPoint(x_pos, y_offset),
                       GPoint(x_pos, y_offset + SECONDS_INDICATOR_HEIGHT_PX));
    x_pos +=
        (SECONDS_INDICATOR_WIDTH_PX + SECONDS_INDICATOR_INTERNAL_PADDING_W_PX);
    seconds_dup--;
  }

  // Draw divider between seconds/battery (only if connected)
  x_pos += INTERNAL_ITEM_PADDING_PX;
  if (is_connected) {
    graphics_fill_rect(
        ctx,
        (GRect){.origin = {x_pos, y_offset + CONN_ICON_OFFSET_INSIDE_ROW_PX},
                .size = {CONNECTED_ICON_W_H_PX, CONNECTED_ICON_W_H_PX}},
        0, GCornerNone);
  }
  x_pos += (CONNECTED_ICON_W_H_PX + INTERNAL_ITEM_PADDING_PX);

  // Draw battery with lines
  if (!is_charging) {
    float batt_pct_across_section = (float)battery_percent / 100.0F;
    int battery_lines = (int)(batt_pct_across_section * 7.0F);
    int battery_x_pos = x_pos;
    while (battery_lines > 0) {
      graphics_draw_line(
          ctx, GPoint(battery_x_pos, y_offset + BATT_OFFSET_INSIDE_ROW_PX),
          GPoint(battery_x_pos,
                 y_offset + BATT_OFFSET_INSIDE_ROW_PX + BATTERY_BAR_HEIGHT_PX));
      battery_x_pos +=
          (BATTERY_BAR_WIDTH_PX + BATTERY_BAR_INTERNAL_PADDING_W_PX);
      battery_lines--;
    }
  } else { // Draw charging image
    if (seconds % 2) {
      graphics_draw_bitmap_in_rect(
          ctx, charging_icon_low_bmp,
          (GRect){.origin = {x_pos, y_offset + BATT_OFFSET_INSIDE_ROW_PX},
                  .size = {CHARGING_ICON_WIDTH_PX, CHARGING_ICON_HEIGHT_PX}});
    } else {
      graphics_draw_bitmap_in_rect(
          ctx, charging_icon_bmp,
          (GRect){.origin = {x_pos, y_offset + BATT_OFFSET_INSIDE_ROW_PX},
                  .size = {CHARGING_ICON_WIDTH_PX, CHARGING_ICON_HEIGHT_PX}});
    }
  }

  y_offset +=
      SECONDS_BATT_CONN_ROW_TOTAL_HEIGHT_PX + EXTERNAL_ITEM_VERTICAL_PADDING_PX;

  // Draw day of week (0-7) (@ y = 107)
  float dow_pct_across_screen = (float)day_of_week / 6.0F;
  int dow_x_position =
      EXTERNAL_ITEM_HORIZONTAL_PADDING_PX +
      (dow_pct_across_screen * SINGLE_WIDE_SMALL_DIGIT_MOVEMENT_WIDTH_PX);
  draw_hex_small(ctx, day_of_week + 1, dow_x_position, y_offset);
  // smear_location(day_of_week + 1, ctx, dow_x_position, 107, 12, 18, 40, 2,
  //                true);
  y_offset += SMALL_DIGIT_HEIGHT_PX + EXTERNAL_ITEM_VERTICAL_PADDING_PX;

  // Draw day of month (0-31) (@ y = 127)
  int dom_hex_1_val = day_of_month / HEX_DIGIT_COUNT;
  int dom_hex_2_val = day_of_month % HEX_DIGIT_COUNT;
  float dom_pct_across_screen = ((float)day_of_month - 1) / 30.0F;
  int dom_x_position =
      EXTERNAL_ITEM_HORIZONTAL_PADDING_PX +
      (dom_pct_across_screen * DOUBLE_WIDE_SMALL_DIGIT_MOVEMENT_WIDTH_PX);
  draw_hex_small(ctx, dom_hex_1_val, dom_x_position, y_offset);
  draw_hex_small(ctx, dom_hex_2_val,
                 dom_x_position + SMALL_DIGIT_WIDTH_PX +
                     INTERNAL_ITEM_PADDING_PX,
                 y_offset);
  // smear_location_two_wide(dom_hex_1_val, dom_hex_2_val, ctx, dom_x_position,
  //                         127, 12, 18, 2, 40, true);
  y_offset += SMALL_DIGIT_HEIGHT_PX + EXTERNAL_ITEM_VERTICAL_PADDING_PX;

  // Draw month (0-11) (@ y = 147)
  float month_pct_across_screen = (float)month / 11.0F;
  int month_x_position =
      EXTERNAL_ITEM_HORIZONTAL_PADDING_PX +
      (month_pct_across_screen * SINGLE_WIDE_SMALL_DIGIT_MOVEMENT_WIDTH_PX);
  draw_hex_small(ctx, month + 1, month_x_position, y_offset);
  // smear_location(month + 1, ctx, m_x_position, 147, 12, 18, 40, 2, true);

  VERBOSE_LOG("layer_update_callback() complete");
}

static void handle_bluetooth(bool connected) {
  is_connected = connected;
  if (settings.DisconnectVibrate && time_running > 5) {
    vibes_long_pulse();
  }
}

static void handle_battery(BatteryChargeState charge_state) {
  is_charging = charge_state.is_charging;
  battery_percent = charge_state.charge_percent;
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  time_running++;
  if (settings.PowerMode || tick_time->tm_sec % 15 == 0) {
    layer_mark_dirty(layer);
  }
}

static void inbox_received_callback(DictionaryIterator *iterator,
                                    void *context) {
  VERBOSE_LOG("Inbox message received");

  Tuple *power_mode_tuple = dict_find(iterator, MESSAGE_KEY_PowerMode);
  Tuple *ghost_time_tuple = dict_find(iterator, MESSAGE_KEY_GhostTime);
  Tuple *ghost_date_tuple = dict_find(iterator, MESSAGE_KEY_GhostDate);
  Tuple *hourly_vibrate_tuple = dict_find(iterator, MESSAGE_KEY_HourlyVibrate);
  Tuple *disconnect_vibrate_tuple =
      dict_find(iterator, MESSAGE_KEY_DisconnectVibrate);
  if (power_mode_tuple) {
    settings.PowerMode = power_mode_tuple->value->int32 == 1;
  }
  if (ghost_time_tuple) {
    settings.GhostTime = ghost_time_tuple->value->int32 == 1;
  }
  if (ghost_date_tuple) {
    settings.GhostDate = ghost_date_tuple->value->int32 == 1;
  }
  if (hourly_vibrate_tuple) {
    settings.HourlyVibrate = hourly_vibrate_tuple->value->int32 == 1;
  }
  if (disconnect_vibrate_tuple) {
    settings.DisconnectVibrate = disconnect_vibrate_tuple->value->int32 == 1;
  }
  if (power_mode_tuple || ghost_time_tuple || ghost_date_tuple ||
      hourly_vibrate_tuple || disconnect_vibrate_tuple) {
    persist_write_data(SETTINGS_KEY, &settings, sizeof(ClaySettings));
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Inbox message dropped: %d", (int)reason);
}

void init() {
  VERBOSE_LOG("Init'ing");

  window = window_create();
  window_stack_push(window, true);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);

  const int inbox_size = 256;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);

  // Defaults, matching config.js
  settings.PowerMode = true;
  settings.GhostTime = true;
  settings.GhostDate = true;
  settings.HourlyVibrate = true;
  settings.DisconnectVibrate = true;
  if (persist_read_data(SETTINGS_KEY, &settings, sizeof(settings)) ==
      E_DOES_NOT_EXIST) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to read settings! Writing defaults");
    persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  }

  // Init the layer for display the image
  window_layer = window_get_root_layer(window);
  bounds = layer_get_frame(window_layer);
  layer = layer_create(bounds);
  layer_set_update_proc(layer, layer_update_callback);
  layer_add_child(window_layer, layer);

  for (int i = 0; i < HEX_DIGIT_COUNT; i++) {
    big_numerals_bmps[i] =
        gbitmap_create_with_resource(BIG_NUMBER_RESOURCE_IDS[i]);
    small_numerals_bmps[i] =
        gbitmap_create_with_resource(SMALL_NUMBER_RESOURCE_IDS[i]);
  }
  charging_icon_bmp = gbitmap_create_with_resource(RESOURCE_ID_CHARGING_ICON);
  charging_icon_low_bmp =
      gbitmap_create_with_resource(RESOURCE_ID_CHARGING_ICON_LOW);
  fades_htl_sz_amnt_bmps[0][0] =
      gbitmap_create_with_resource(RESOURCE_ID_SMEAR_50);
  fades_htl_sz_amnt_bmps[0][1] =
      gbitmap_create_with_resource(RESOURCE_ID_SMEAR_25);
  fades_htl_sz_amnt_bmps[1][0] =
      gbitmap_create_with_resource(RESOURCE_ID_SMEAR_SMALL_50);
  fades_htl_sz_amnt_bmps[1][1] =
      gbitmap_create_with_resource(RESOURCE_ID_SMEAR_SMALL_25);

  VERBOSE_LOG("init'd windows + all resources");

  srand(time(NULL));

  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek());
  handle_battery(battery_state_service_peek());

  layer_mark_dirty(layer);

  VERBOSE_LOG("Done init'ing");
}

void deinit() {
  VERBOSE_LOG("Deinit'ing");

  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();

  for (int i = 0; i < HEX_DIGIT_COUNT; i++) {
    gbitmap_destroy(big_numerals_bmps[i]);
    gbitmap_destroy(small_numerals_bmps[i]);
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      gbitmap_destroy(fades_htl_sz_amnt_bmps[i][j]);
    }
  }
  gbitmap_destroy(charging_icon_bmp);
  gbitmap_destroy(charging_icon_low_bmp);

  window_destroy(window);
  layer_destroy(layer);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}