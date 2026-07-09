// Minimally updated version of a smartwatch I built when I was a student
// Be warned: Some questionable stuff here! Everything egregious was fixed,
// but it still isn't exactly the height of clean code

#include "common.h"
#include "pebble.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

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
#define BIG_DIGIT_NOISE_PX_COUNT 160 // ~15%
#define SMALL_DIGIT_WIDTH_PX 12
#define SMALL_DIGIT_HEIGHT_PX 18
#define SMALL_DIGIT_NOISE_PX_COUNT 27 // ~15%

#define NOISE_SRC_LEN 512

// Seconds - Connected - Battery
#define SECONDS_BATT_CONN_ROW_TOTAL_HEIGHT_PX 8
#define SECONDS_INDICATOR_HEIGHT_PX SECONDS_BATT_CONN_ROW_TOTAL_HEIGHT_PX
#define SECONDS_INDICATOR_WIDTH_PX 1
#define SECONDS_INDICATOR_INTERNAL_PADDING_W_PX                                \
  (PBL_PLATFORM_TYPE_CURRENT == PlatformTypeEmery ? 2 : 1)
#define SECONDS_INDICATOR_TOTAL_WIDTH_PX                                       \
  ((SECONDS_INDICATOR_WIDTH_PX * 60) +                                         \
   (SECONDS_INDICATOR_INTERNAL_PADDING_W_PX * 59))
#define CONNECTED_ICON_W_H_PX 3
#define CONN_ICON_OFFSET_INSIDE_ROW_PX                                         \
  ((SECONDS_BATT_CONN_ROW_TOTAL_HEIGHT_PX - CONNECTED_ICON_W_H_PX) / 2) + 1
#define BATTERY_BAR_WIDTH_PX 1
#define BATTERY_BAR_HEIGHT_PX 4
#define BATTERY_BAR_INTERNAL_PADDING_W_PX 1
#define CHARGING_ICON_WIDTH_PX 9
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
#define INTERNAL_FONT_PADDING_PX 4
#define EXTERNAL_ITEM_HORIZONTAL_PADDING_PX                                    \
  (PBL_PLATFORM_TYPE_CURRENT == PlatformTypeEmery ? 8 : 2)
#define EXTERNAL_ITEM_VERTICAL_PADDING_PX                                      \
  (EXTERNAL_PADDING_TOTAL_PX / (ROWS_OF_CONTENT + 2))
// EXTERNAL_ITEM_PADDING_PX resolves to 9.5 on PT2, integer rounding down to 9
// This "throws out" (.5 * 8) pixels that we have to account for, so we pad in
// an extra 5 pixels on top
// On everything else, it resolves to 2
#define EXTRA_VERTICAL_PADDING_PX                                              \
  (PBL_PLATFORM_TYPE_CURRENT == PlatformTypeEmery ? 5 : 0)
#define HORIZONTAL_TRACK_WIDTH_PX                                              \
  (PBL_DISPLAY_WIDTH - (2 * EXTERNAL_ITEM_HORIZONTAL_PADDING_PX))
#define SINGLE_WIDE_BIG_DIGIT_MOVEMENT_WIDTH_PX                                \
  (HORIZONTAL_TRACK_WIDTH_PX - BIG_DIGIT_WIDTH_PX)
#define DOUBLE_WIDE_BIG_DIGIT_MOVEMENT_WIDTH_PX                                \
  (HORIZONTAL_TRACK_WIDTH_PX - (BIG_DIGIT_WIDTH_PX * 2) -                      \
   INTERNAL_FONT_PADDING_PX)
#define SINGLE_WIDE_SMALL_DIGIT_MOVEMENT_WIDTH_PX                              \
  (HORIZONTAL_TRACK_WIDTH_PX - SMALL_DIGIT_WIDTH_PX)
#define DOUBLE_WIDE_SMALL_DIGIT_MOVEMENT_WIDTH_PX                              \
  (HORIZONTAL_TRACK_WIDTH_PX - (SMALL_DIGIT_WIDTH_PX * 2) -                    \
   INTERNAL_FONT_PADDING_PX)

#define BIG_HEX_DIGIT_COUNT 16
#define SMALL_HEX_DIGIT_COUNT 24

#define SETTINGS_KEY 1

const GColor FOREGROUND_COLOR = GColorWhite;
const GColor BACKGROUND_COLOR = GColorBlack;
const GColor CONNECTED_INDICATOR_COLOR =
    PBL_IF_COLOR_ELSE(GColorSunsetOrange, FOREGROUND_COLOR);
const GColor BATTERY_COLOR =
    PBL_IF_COLOR_ELSE(GColorMintGreen, FOREGROUND_COLOR);

// R G B
GColor MINUTES_SACCADE[] = {GColorRed, GColorFolly, GColorDarkCandyAppleRed};
GColor HOURS_SACCADE[] = {GColorElectricBlue, GColorVividCerulean,
                          GColorCobaltBlue};
GColor DAY_OF_WEEK_SACCADE[] = {GColorWhite, GColorLightGray, GColorDarkGray};
GColor MONTH_OF_YEAR_SACCADE[] = {GColorScreaminGreen, GColorIslamicGreen,
                                  GColorDarkGreen};
GColor DAY_OF_MONTH_SACCADE[] = {GColorSunsetOrange, GColorOrange,
                                 GColorDarkCandyAppleRed};

const int BIG_NUMBER_RESOURCE_IDS[] = {
    RESOURCE_ID_NUMBER_0, RESOURCE_ID_NUMBER_1, RESOURCE_ID_NUMBER_2,
    RESOURCE_ID_NUMBER_3, RESOURCE_ID_NUMBER_4, RESOURCE_ID_NUMBER_5,
    RESOURCE_ID_NUMBER_6, RESOURCE_ID_NUMBER_7, RESOURCE_ID_NUMBER_8,
    RESOURCE_ID_NUMBER_9, RESOURCE_ID_NUMBER_A, RESOURCE_ID_NUMBER_B,
    RESOURCE_ID_NUMBER_C, RESOURCE_ID_NUMBER_D, RESOURCE_ID_NUMBER_E,
    RESOURCE_ID_NUMBER_F};
const int SMALL_NUMBER_RESOURCE_IDS[] = {
    RESOURCE_ID_SMALL_NUMBER_0, RESOURCE_ID_SMALL_NUMBER_1,   // 0 1
    RESOURCE_ID_SMALL_NUMBER_2, RESOURCE_ID_SMALL_NUMBER_3,   // 2 3
    RESOURCE_ID_SMALL_NUMBER_4, RESOURCE_ID_SMALL_NUMBER_5,   // 4 5
    RESOURCE_ID_SMALL_NUMBER_6, RESOURCE_ID_SMALL_NUMBER_7,   // 6 7
    RESOURCE_ID_SMALL_NUMBER_8, RESOURCE_ID_SMALL_NUMBER_9,   // 8 9
    RESOURCE_ID_SMALL_NUMBER_A, RESOURCE_ID_SMALL_NUMBER_B,   // 10 11
    RESOURCE_ID_SMALL_NUMBER_C, RESOURCE_ID_SMALL_NUMBER_D,   // 12 13
    RESOURCE_ID_SMALL_NUMBER_E, RESOURCE_ID_SMALL_NUMBER_F,   // 14 15
    RESOURCE_ID_SMALL_NUMBER_H, RESOURCE_ID_SMALL_NUMBER_M,   // 16 17
    RESOURCE_ID_SMALL_NUMBER_O, RESOURCE_ID_SMALL_NUMBER_R,   // 18 19
    RESOURCE_ID_SMALL_NUMBER_S, RESOURCE_ID_SMALL_NUMBER_T,   // 20 21
    RESOURCE_ID_SMALL_NUMBER_U, RESOURCE_ID_SMALL_NUMBER_W};  // 22 23
const int DAYS_OF_WEEK_RESOURCE_ID_INDICES[][7] = {{20, 22},  // SU
                                                   {17, 18},  // MO
                                                   {21, 22},  // TU
                                                   {23, 14},  // WE
                                                   {21, 16},  // TH
                                                   {15, 19},  // FR
                                                   {20, 10}}; // SA

typedef struct ClaySettings {
  bool HexMode;
  bool PowerMode;
  bool ColorizeDigits;
  bool GhostTime;
  bool GhostDate;
  bool HourlyVibrate;
  bool DisconnectVibrate;
} ClaySettings;

typedef enum RowType {
  HOUR_OF_DAY,
  MINUTE_OF_HOUR,
  DAY_OF_WEEK,
  DAY_OF_MONTH,
  MONTH_OF_YEAR
} RowType;

static Window *window;
static Layer *layer;
static Layer *window_layer;
ClaySettings settings;
static GRect bounds;

static GBitmap *big_numerals_bmps[BIG_HEX_DIGIT_COUNT];
static GBitmap *small_numerals_bmps[SMALL_HEX_DIGIT_COUNT];
static GBitmap *fades_htl_sz_amnt_bmps[2][2];
static GBitmap *charging_icon_bmp;
static GBitmap *charging_icon_low_bmp;

int rand_noise_src[NOISE_SRC_LEN]; // 16 bits * 512 = ~8KB

long time_running = 0L;
int is_charging = 0;
bool is_connected = true;
int battery_percent = 0;

static void draw_hex_char(GContext *ctx, int value, int x, int y, int w, int h,
                          bool is_small, GColor color) {
  const GBitmap *bmp =
      is_small ? small_numerals_bmps[value] : big_numerals_bmps[value];
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  GColor *palette = gbitmap_get_palette(bmp);
  palette[0] = settings.ColorizeDigits
                   ? PBL_IF_COLOR_ELSE(color, FOREGROUND_COLOR)
                   : FOREGROUND_COLOR;
  palette[1] = GColorClear;
  graphics_draw_bitmap_in_rect(ctx, bmp,
                               (GRect){.origin = {x, y}, .size = {w, h}});
}

static void smear_character_at_position(GContext *ctx, int x, int y,
                                        bool is_small, bool is_more_smeared) {
  const int char_w = is_small ? SMALL_DIGIT_WIDTH_PX : BIG_DIGIT_WIDTH_PX;
  const int char_h = is_small ? SMALL_DIGIT_HEIGHT_PX : BIG_DIGIT_HEIGHT_PX;
  GBitmap *smear_bmp =
      fades_htl_sz_amnt_bmps[is_small ? 1 : 0][is_more_smeared ? 1 : 0];
  // smear_bmp is a palettized 1 bit BMP
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(
      ctx, smear_bmp, (GRect){.origin = {x, y}, .size = {char_w, char_h}});

  // So the smear isn't uniform, draw some BACKGROUND_COLOR dots
  // on top of it
  int noise_dots =
      is_small ? SMALL_DIGIT_NOISE_PX_COUNT : BIG_DIGIT_NOISE_PX_COUNT;
  int noise_src_idx = rand();
  graphics_context_set_stroke_color(ctx, BACKGROUND_COLOR);
  while (noise_dots > 0) {
    const int x_dot =
        x + ((rand_noise_src[noise_src_idx % NOISE_SRC_LEN]) % char_w);
    noise_src_idx++;
    const int y_dot =
        y + ((rand_noise_src[noise_src_idx % NOISE_SRC_LEN]) % char_h);
    graphics_draw_pixel(ctx, (GPoint){.x = x_dot, .y = y_dot});
    noise_src_idx++;
    noise_dots--;
  }
}

static void render_hex_value(GContext *ctx, int value, int x, int y,
                             bool use_small_font, bool is_double_wide,
                             GColor color) {
  bool is_rendering_as_hexadecimal = settings.HexMode;
  const int char_w = use_small_font ? SMALL_DIGIT_WIDTH_PX : BIG_DIGIT_WIDTH_PX;
  const int char_h =
      use_small_font ? SMALL_DIGIT_HEIGHT_PX : BIG_DIGIT_HEIGHT_PX;
  const int break_digit = is_rendering_as_hexadecimal ? 16 : 10;
  if (is_double_wide) {
    draw_hex_char(ctx, value / break_digit, x, y, char_w, char_h,
                  use_small_font, color);
    draw_hex_char(ctx, value % break_digit,
                  x + char_w + INTERNAL_FONT_PADDING_PX, y, char_w, char_h,
                  use_small_font, color);
  } else {
    draw_hex_char(ctx, value, x, y, char_w, char_h, use_small_font, color);
  }
}

static void render_day_of_week_non_hex(GContext *ctx, int day_of_week, int x,
                                       int y, GColor color) {
  const int *id_indices = DAYS_OF_WEEK_RESOURCE_ID_INDICES[day_of_week - 1];
  const int left_idx = id_indices[0];
  const int right_idx = id_indices[1];
  draw_hex_char(ctx, left_idx, x, y, SMALL_DIGIT_WIDTH_PX,
                SMALL_DIGIT_HEIGHT_PX, true, color);
  draw_hex_char(ctx, right_idx,
                x + SMALL_DIGIT_WIDTH_PX + INTERNAL_FONT_PADDING_PX, y,
                SMALL_DIGIT_WIDTH_PX, SMALL_DIGIT_HEIGHT_PX, true, color);
}

static void render_hex_row(GContext *ctx, int value, int x, int y,
                           bool use_small_font, RowType row_type) {

  GColor(*color_saccade)[3];
  switch (row_type) {
  case HOUR_OF_DAY:
    color_saccade = &HOURS_SACCADE;
    break;
  case MINUTE_OF_HOUR:
    color_saccade = &MINUTES_SACCADE;
    break;
  case DAY_OF_WEEK:
    color_saccade = &DAY_OF_WEEK_SACCADE;
    break;
  case DAY_OF_MONTH:
    color_saccade = &DAY_OF_MONTH_SACCADE;
    break;
  case MONTH_OF_YEAR:
    color_saccade = &MONTH_OF_YEAR_SACCADE;
    break;
  }

  bool is_rendering_as_hexadecimal = settings.HexMode;
  bool is_special_case_day_of_week_render =
      row_type == DAY_OF_WEEK && !is_rendering_as_hexadecimal;
  const int char_w = use_small_font ? SMALL_DIGIT_WIDTH_PX : BIG_DIGIT_WIDTH_PX;
  const bool is_double_wide =
      (is_rendering_as_hexadecimal
           ? value >= 16
           : (value >= 10 || row_type == MINUTE_OF_HOUR)) ||
      is_special_case_day_of_week_render;
  const int total_block_w = (char_w * (is_double_wide ? 2 : 1)) +
                            (is_double_wide ? INTERNAL_FONT_PADDING_PX : 0);

  // First pass: the actual display value
  if (!is_special_case_day_of_week_render) {
    render_hex_value(ctx, value, x, y, use_small_font, is_double_wide,
                     (*color_saccade)[0]);
  } else {
    render_day_of_week_non_hex(ctx, value, x, y, (*color_saccade)[0]);
  }

  bool should_ghost = (settings.GhostDate && use_small_font) ||
                      (settings.GhostTime && !use_small_font);
  if (!should_ghost) {
    return;
  }

  const int jump_left_amount = total_block_w + INTERNAL_FONT_PADDING_PX;
  int i = 1;
  x -= jump_left_amount;
  while (x >= -total_block_w) {
    if (!is_special_case_day_of_week_render) {
      render_hex_value(ctx, value, x, y, use_small_font, is_double_wide,
                       (*color_saccade)[i]);
    } else {
      render_day_of_week_non_hex(ctx, value, x, y, (*color_saccade)[i]);
    }
    smear_character_at_position(ctx, x, y, use_small_font, true);
    if (is_double_wide) {
      smear_character_at_position(ctx, x + INTERNAL_FONT_PADDING_PX + char_w, y,
                                  use_small_font, true);
    }
    x -= jump_left_amount;
    i++;
    if (i > 2) {
      i = 0;
    }
  }
}

static void layer_update_callback(Layer *me, GContext *ctx) {
  VERBOSE_LOG("layer_update_callback()");

  time_t in_time_units;
  time(&in_time_units);
  struct tm *curr_time = localtime(&in_time_units);

  // Time values are 0 aligned
  int hours = curr_time->tm_hour;  // 0 - 23
  int minutes = curr_time->tm_min; // 0 - 59
  int seconds = curr_time->tm_sec; // 0 - 59
  // Date values are 1 aligned
  int month = curr_time->tm_mon + 1;        // 1 - 12
  int day_of_month = curr_time->tm_mday;    // 1 - 31
  int day_of_week = curr_time->tm_wday + 1; // 1 - 7
  bool is_24_hour_style = clock_is_24h_style();

  VERBOSE_LOG("Time is: %d:%d:%d", hours, minutes, seconds);

  if (settings.HourlyVibrate && minutes == 0 && seconds == 0) {
    vibes_long_pulse();
  }

  // Watchface is white-on-black
  graphics_context_set_fill_color(ctx, BACKGROUND_COLOR);
  graphics_fill_rect(ctx,
                     (GRect){.origin = {0, 0},
                             .size = {PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT}},
                     0, GCornerNone);

  graphics_context_set_fill_color(ctx, FOREGROUND_COLOR);

  int y_offset = EXTERNAL_ITEM_VERTICAL_PADDING_PX + EXTRA_VERTICAL_PADDING_PX;

  // Draw hours in hex
  int hour_value = hours;
  if (!is_24_hour_style && hour_value > 11) {
    hour_value -= 12;
  }
  float hour_pct_across_screen = (float)hours / 23.0F;
  int hour_x_position = EXTERNAL_ITEM_HORIZONTAL_PADDING_PX +
                        (hour_pct_across_screen *
                         (is_24_hour_style || !settings.HexMode
                              ? DOUBLE_WIDE_BIG_DIGIT_MOVEMENT_WIDTH_PX
                              : SINGLE_WIDE_BIG_DIGIT_MOVEMENT_WIDTH_PX));
  render_hex_row(ctx, hour_value, hour_x_position, y_offset, false,
                 HOUR_OF_DAY);
  y_offset += BIG_DIGIT_HEIGHT_PX + EXTERNAL_ITEM_VERTICAL_PADDING_PX;

  // Draw the minutes in hex
  float min_pct_across_screen = (float)minutes / 59.0F;
  int min_x_position =
      EXTERNAL_ITEM_HORIZONTAL_PADDING_PX +
      (min_pct_across_screen * DOUBLE_WIDE_BIG_DIGIT_MOVEMENT_WIDTH_PX);
  render_hex_row(ctx, minutes, min_x_position, y_offset, false, MINUTE_OF_HOUR);
  y_offset += BIG_DIGIT_HEIGHT_PX + EXTERNAL_ITEM_VERTICAL_PADDING_PX;

  // Draw the seconds with lines
  graphics_context_set_stroke_color(ctx, FOREGROUND_COLOR);
  int x_pos = SECONDS_BATT_CONN_ROW_LEFT_OFFSET_PX;
  int seconds_dup = seconds;
  while (seconds_dup > 0) {
    graphics_draw_line(ctx, GPoint(x_pos, y_offset),
                       GPoint(x_pos, y_offset + SECONDS_INDICATOR_HEIGHT_PX));
    x_pos +=
        (SECONDS_INDICATOR_WIDTH_PX + SECONDS_INDICATOR_INTERNAL_PADDING_W_PX);
    seconds_dup--;
  }

  // Draw dot between seconds/battery (only if connected)
  x_pos = SECONDS_BATT_CONN_ROW_LEFT_OFFSET_PX +
          SECONDS_INDICATOR_TOTAL_WIDTH_PX + INTERNAL_ITEM_PADDING_PX;
  if (is_connected) {
    graphics_context_set_fill_color(ctx, CONNECTED_INDICATOR_COLOR);
    graphics_fill_rect(
        ctx,
        (GRect){.origin = {x_pos, y_offset + CONN_ICON_OFFSET_INSIDE_ROW_PX},
                .size = {CONNECTED_ICON_W_H_PX, CONNECTED_ICON_W_H_PX}},
        1, GCornersAll);
  }
  graphics_context_set_fill_color(ctx, FOREGROUND_COLOR);
  x_pos += (CONNECTED_ICON_W_H_PX + INTERNAL_ITEM_PADDING_PX);

  // Draw battery with lines
  graphics_context_set_stroke_color(ctx, BATTERY_COLOR);
  if (!is_charging) {
    float batt_pct_across_section = (float)battery_percent / 100.0F;
    int battery_lines = (int)round(batt_pct_across_section * 5.0F);
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
    graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
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
  graphics_context_set_stroke_color(ctx, FOREGROUND_COLOR);

  y_offset +=
      SECONDS_BATT_CONN_ROW_TOTAL_HEIGHT_PX + EXTERNAL_ITEM_VERTICAL_PADDING_PX;

  // Draw day of week (1 - 7)
  float dow_pct_across_screen = (float)day_of_week / 7.0F;
  int dow_x_position =
      EXTERNAL_ITEM_HORIZONTAL_PADDING_PX +
      (dow_pct_across_screen * SINGLE_WIDE_SMALL_DIGIT_MOVEMENT_WIDTH_PX);
  render_hex_row(ctx, day_of_week, dow_x_position, y_offset, true, DAY_OF_WEEK);
  y_offset += SMALL_DIGIT_HEIGHT_PX + EXTERNAL_ITEM_VERTICAL_PADDING_PX;

  // Draw month (1 - 12)
  float month_pct_across_screen = (float)month / 12.0F;
  int month_x_position =
      EXTERNAL_ITEM_HORIZONTAL_PADDING_PX +
      (month_pct_across_screen * SINGLE_WIDE_SMALL_DIGIT_MOVEMENT_WIDTH_PX);
  render_hex_row(ctx, month, month_x_position, y_offset, true, MONTH_OF_YEAR);
  y_offset += SMALL_DIGIT_HEIGHT_PX + EXTERNAL_ITEM_VERTICAL_PADDING_PX;

  // Draw day of month (1 - 31)
  float dom_pct_across_screen = (float)day_of_month / 31.0F;
  int dom_x_position =
      EXTERNAL_ITEM_HORIZONTAL_PADDING_PX +
      (dom_pct_across_screen * DOUBLE_WIDE_SMALL_DIGIT_MOVEMENT_WIDTH_PX);
  render_hex_row(ctx, day_of_month, dom_x_position, y_offset, true,
                 DAY_OF_MONTH);

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
  Tuple *hex_mode_tuple = dict_find(iterator, MESSAGE_KEY_HexMode);
  Tuple *power_mode_tuple = dict_find(iterator, MESSAGE_KEY_PowerMode);
  Tuple *colorize_digits_tuple =
      dict_find(iterator, MESSAGE_KEY_ColorizeDigits);
  Tuple *ghost_time_tuple = dict_find(iterator, MESSAGE_KEY_GhostTime);
  Tuple *ghost_date_tuple = dict_find(iterator, MESSAGE_KEY_GhostDate);
  Tuple *hourly_vibrate_tuple = dict_find(iterator, MESSAGE_KEY_HourlyVibrate);
  Tuple *disconnect_vibrate_tuple =
      dict_find(iterator, MESSAGE_KEY_DisconnectVibrate);
  if (hex_mode_tuple) {
    settings.HexMode = hex_mode_tuple->value->int32 == 1;
  }
  if (power_mode_tuple) {
    settings.PowerMode = power_mode_tuple->value->int32 == 1;
  }
  if (colorize_digits_tuple) {
    settings.ColorizeDigits = colorize_digits_tuple->value->int32 == 1;
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
  if (hex_mode_tuple || power_mode_tuple || colorize_digits_tuple ||
      ghost_time_tuple || ghost_date_tuple || hourly_vibrate_tuple ||
      disconnect_vibrate_tuple) {
    persist_write_data(SETTINGS_KEY, &settings, sizeof(ClaySettings));
    layer_mark_dirty(layer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Inbox message dropped: %d", (int)reason);
}

static GBitmap *load_resource_with_fg_color(int res_id, GColor color) {
  GBitmap *res_bmp = gbitmap_create_with_resource(res_id);
  GBitmap *bmp_copy = gbitmap_create_palettized_from_1bit(res_bmp);
  GColor *palette = gbitmap_get_palette(bmp_copy);
  palette[0] = GColorClear;
  palette[1] = color;
  gbitmap_destroy(res_bmp);
  return bmp_copy;
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
  settings.HexMode = true;
  settings.PowerMode = true;
  settings.ColorizeDigits = false;
  settings.GhostTime = true;
  settings.GhostDate = false;
  settings.HourlyVibrate = false;
  settings.DisconnectVibrate = true;
  // If more keys were added post-release, this would need to be updated
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

  for (int i = 0; i < BIG_HEX_DIGIT_COUNT; i++) {
    big_numerals_bmps[i] = load_resource_with_fg_color(
        BIG_NUMBER_RESOURCE_IDS[i], FOREGROUND_COLOR);
  }
  for (int i = 0; i < SMALL_HEX_DIGIT_COUNT; i++) {
    small_numerals_bmps[i] = load_resource_with_fg_color(
        SMALL_NUMBER_RESOURCE_IDS[i], FOREGROUND_COLOR);
  }
  charging_icon_bmp = gbitmap_create_with_resource(RESOURCE_ID_CHARGING_ICON);
  charging_icon_low_bmp =
      gbitmap_create_with_resource(RESOURCE_ID_CHARGING_ICON_LOW);
  fades_htl_sz_amnt_bmps[0][0] =
      load_resource_with_fg_color(RESOURCE_ID_SMEAR_25, BACKGROUND_COLOR);
  fades_htl_sz_amnt_bmps[0][1] =
      load_resource_with_fg_color(RESOURCE_ID_SMEAR_125, BACKGROUND_COLOR);
  fades_htl_sz_amnt_bmps[1][0] =
      load_resource_with_fg_color(RESOURCE_ID_SMEAR_SMALL_25, BACKGROUND_COLOR);
  fades_htl_sz_amnt_bmps[1][1] = load_resource_with_fg_color(
      RESOURCE_ID_SMEAR_SMALL_125, BACKGROUND_COLOR);

  for (int i = 0; i < NOISE_SRC_LEN; i++) {
    rand_noise_src[i] = rand();
  }

  VERBOSE_LOG("Init'd all resources");

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

  for (int i = 0; i < BIG_HEX_DIGIT_COUNT; i++) {
    gbitmap_destroy(big_numerals_bmps[i]);
  }
  for (int i = 0; i < SMALL_HEX_DIGIT_COUNT; i++) {
    gbitmap_destroy(small_numerals_bmps[i]);
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      gbitmap_destroy(fades_htl_sz_amnt_bmps[i][j]);
    }
  }
  gbitmap_destroy(charging_icon_bmp);
  gbitmap_destroy(charging_icon_low_bmp);

  layer_remove_child_layers(window_layer);
  layer_destroy(layer);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}