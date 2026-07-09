#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    EVENT_NONE = 0,
    EVENT_ACTIVATE,
    EVENT_TICK,
    EVENT_LOW_ENERGY_UPDATE,
    EVENT_BACKGROUND_TASK,
    EVENT_TIMEOUT,
    EVENT_LIGHT_BUTTON_DOWN,
    EVENT_LIGHT_BUTTON_UP,
    EVENT_LIGHT_LONG_PRESS,
    EVENT_LIGHT_LONG_UP,
    EVENT_LIGHT_REALLY_LONG_PRESS,
    EVENT_MODE_BUTTON_DOWN,
    EVENT_MODE_BUTTON_UP,
    EVENT_MODE_LONG_PRESS,
    EVENT_MODE_LONG_UP,
    EVENT_MODE_REALLY_LONG_PRESS,
    EVENT_ALARM_BUTTON_DOWN,
    EVENT_ALARM_BUTTON_UP,
    EVENT_ALARM_LONG_PRESS,
    EVENT_ALARM_LONG_UP,
    EVENT_ALARM_REALLY_LONG_PRESS,
    EVENT_ACCELEROMETER_WAKE,
    EVENT_SINGLE_TAP,
    EVENT_DOUBLE_TAP,
} movement_event_type_t;

typedef struct {
    uint8_t event_type;
    uint8_t subsecond;
} movement_event_t;

typedef union {
    struct {
        uint8_t second;
        uint8_t minute;
        uint8_t hour;
        uint8_t day;
        uint8_t month;
        uint8_t year;
    } unit;
    uint32_t reg;
} watch_date_time_t;

typedef enum {
    WATCH_POSITION_FULL = 0,
    WATCH_POSITION_TOP,
    WATCH_POSITION_TOP_LEFT,
    WATCH_POSITION_TOP_RIGHT,
    WATCH_POSITION_BOTTOM,
    WATCH_POSITION_HOURS,
    WATCH_POSITION_MINUTES,
    WATCH_POSITION_SECONDS,
} watch_position_t;

typedef enum {
    WATCH_INDICATOR_SIGNAL = 0,
    WATCH_INDICATOR_BELL,
    WATCH_INDICATOR_PM,
    WATCH_INDICATOR_24H,
} watch_indicator_t;

typedef enum {
    WATCH_LCD_TYPE_UNKNOWN = 0,
    WATCH_LCD_TYPE_CLASSIC = 0b10101001,
    WATCH_LCD_TYPE_CUSTOM = 0b01010110,
} watch_lcd_type_t;

typedef union {
    struct {
        int16_t latitude : 16;
        int16_t longitude : 16;
    } bit;
    uint32_t reg;
} movement_location_t;

typedef struct {
    uint8_t wants_background_task: 1;
    uint8_t has_active_alarm: 1;
    uint8_t responds_to_dst_change: 1;
} movement_watch_face_advisory_t;

typedef void (*watch_face_setup)(uint8_t watch_face_index, void **context_ptr);
typedef void (*watch_face_activate)(void *context);
typedef bool (*watch_face_loop)(movement_event_t event, void *context);
typedef void (*watch_face_resign)(void *context);
typedef movement_watch_face_advisory_t (*watch_face_advise)(void *context);

typedef struct {
    watch_face_setup setup;
    watch_face_activate activate;
    watch_face_loop loop;
    watch_face_resign resign;
    watch_face_advise advise;
} watch_face_t;

watch_date_time_t movement_get_local_date_time(void);
int32_t movement_get_timezone_offset_for_date(watch_date_time_t date_time);
void watch_display_text(watch_position_t location, const char *string);
void watch_set_indicator(watch_indicator_t indicator);
void watch_clear_indicator(watch_indicator_t indicator);
bool movement_default_loop_handler(movement_event_t event);
void movement_request_tick_frequency(uint8_t freq);
