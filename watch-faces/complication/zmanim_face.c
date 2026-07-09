/*
 * MIT License
 *
 * Copyright (c) 2026
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "zmanim_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_common_display.h"
#include "location_settings.h"
#include "sunriset.h"

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define ZMANIM_COUNT 12
#define ZMANIM_DAWN_DEGREES -16.1
#define ZMANIM_TALIS_DEGREES -10.2
#define ZMANIM_NIGHTFALL_3_STARS_DEGREES -8.5

typedef struct {
    char label[7];
    int16_t minute;
} zmanim_entry_t;

static const uint8_t _zmanim_location_count = sizeof(zmanimLongLatPresets) / sizeof(zmanim_long_lat_presets_t);

static movement_location_t _zmanim_get_location(zmanim_state_t *state) {
    movement_location_t movement_location = {0};

    if (state->longLatToUse == 0 || _zmanim_location_count <= 1) {
        movement_location = location_settings_load_location();
    } else {
        movement_location.bit.latitude = zmanimLongLatPresets[state->longLatToUse].latitude;
        movement_location.bit.longitude = zmanimLongLatPresets[state->longLatToUse].longitude;
    }

    return movement_location;
}

static int16_t _zmanim_local_minute_from_hours(double hours) {
    int16_t minutes = (int16_t)floor(hours * 60.0 + 0.5);

    while (minutes < 0) minutes += 1440;
    while (minutes >= 1440) minutes -= 1440;

    return minutes;
}

static void _zmanim_format_time(char *buf, int16_t minute) {
    watch_date_time_t scratch_time = {0};
    uint8_t hour = minute / 60;
    uint8_t min = minute % 60;

    if (!movement_clock_mode_24h()) {
        scratch_time.unit.hour = hour;
        scratch_time.unit.minute = min;
        if (watch_utility_convert_to_12_hour(&scratch_time)) watch_set_indicator(WATCH_INDICATOR_PM);
        else watch_clear_indicator(WATCH_INDICATOR_PM);
        hour = scratch_time.unit.hour;
    } else {
        watch_set_indicator(WATCH_INDICATOR_24H);
        watch_clear_indicator(WATCH_INDICATOR_PM);
    }

    buf[0] = hour < 10 ? ' ' : '0' + (hour / 10);
    buf[1] = '0' + (hour % 10);
    buf[2] = '0' + (min / 10);
    buf[3] = '0' + (min % 10);
    buf[4] = ' ';
    buf[5] = ' ';
    buf[6] = '\0';
}

static bool _zmanim_compute(zmanim_state_t *state, zmanim_entry_t *zmanim) {
    movement_location_t movement_location = _zmanim_get_location(state);

    if (movement_location.reg == 0) return false;

    watch_date_time_t date_time = movement_get_local_date_time();
    int16_t lat_centi = (int16_t)movement_location.bit.latitude;
    int16_t lon_centi = (int16_t)movement_location.bit.longitude;
    double lat = (double)lat_centi / 100.0;
    double lon = (double)lon_centi / 100.0;
    double hours_from_utc = ((double)movement_get_timezone_offset_for_date(date_time)) / 3600.0;
    double sunrise, sunset, dawn, night_16_1, talis, night_3_stars_start, night_3_stars;
    uint16_t year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR;

    if (sun_rise_set(year, date_time.unit.month, date_time.unit.day, lon, lat, &sunrise, &sunset) != 0) return false;
    if (__sunriset__(year, date_time.unit.month, date_time.unit.day, lon, lat, ZMANIM_DAWN_DEGREES, 0, &dawn, &night_16_1) != 0) return false;
    if (__sunriset__(year, date_time.unit.month, date_time.unit.day, lon, lat, ZMANIM_TALIS_DEGREES, 0, &talis, &night_3_stars_start) != 0) return false;
    if (__sunriset__(year, date_time.unit.month, date_time.unit.day, lon, lat, ZMANIM_NIGHTFALL_3_STARS_DEGREES, 0, &night_3_stars_start, &night_3_stars) != 0) return false;

    sunrise += hours_from_utc;
    sunset += hours_from_utc;
    dawn += hours_from_utc;
    night_16_1 += hours_from_utc;
    talis += hours_from_utc;
    night_3_stars += hours_from_utc;

    double gra_day = sunset - sunrise;
    double magen_avraham_day = night_16_1 - dawn;

    if (gra_day < 0) gra_day += 24.0;
    if (magen_avraham_day < 0) magen_avraham_day += 24.0;

    memcpy(zmanim[0].label, " AlOt ", 7);
    zmanim[0].minute = _zmanim_local_minute_from_hours(dawn);
    memcpy(zmanim[1].label, " TALIS", 7);
    zmanim[1].minute = _zmanim_local_minute_from_hours(talis);
    memcpy(zmanim[2].label, " NEtZ ", 7);
    zmanim[2].minute = _zmanim_local_minute_from_hours(sunrise);
    memcpy(zmanim[3].label, "MA SHM", 7);
    zmanim[3].minute = _zmanim_local_minute_from_hours(dawn + magen_avraham_day / 4.0);
    memcpy(zmanim[4].label, "GRA SH", 7);
    zmanim[4].minute = _zmanim_local_minute_from_hours(sunrise + gra_day / 4.0);
    memcpy(zmanim[5].label, "SHACH ", 7);
    zmanim[5].minute = _zmanim_local_minute_from_hours(sunrise + gra_day / 3.0);
    memcpy(zmanim[6].label, "Chatzo", 7);
    zmanim[6].minute = _zmanim_local_minute_from_hours(sunrise + gra_day / 2.0);
    memcpy(zmanim[7].label, "MInCHA", 7);
    zmanim[7].minute = _zmanim_local_minute_from_hours(sunrise + gra_day / 2.0 + gra_day / 24.0);
    memcpy(zmanim[8].label, " PlAG ", 7);
    zmanim[8].minute = _zmanim_local_minute_from_hours(sunset - gra_day * 1.25 / 12.0);
    memcpy(zmanim[9].label, " SHKIA", 7);
    zmanim[9].minute = _zmanim_local_minute_from_hours(sunset);
    memcpy(zmanim[10].label, "3STAR ", 7);
    zmanim[10].minute = _zmanim_local_minute_from_hours(night_3_stars);
    memcpy(zmanim[11].label, " 72MIn", 7);
    zmanim[11].minute = _zmanim_local_minute_from_hours(sunset + 1.2);

    return true;
}

static void _zmanim_face_update(zmanim_state_t *state) {
    char buf[7];
    zmanim_entry_t zmanim[ZMANIM_COUNT];

    watch_clear_colon();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "ZM", "ZM");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");

    if (!_zmanim_compute(state, zmanim)) {
        watch_clear_indicator(WATCH_INDICATOR_PM);
        watch_clear_indicator(WATCH_INDICATOR_24H);
        watch_display_text(WATCH_POSITION_BOTTOM, "No Loc");
        return;
    }

    if (state->showing_time) {
        watch_set_colon();
        _zmanim_format_time(buf, zmanim[state->zman_index].minute);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        watch_clear_colon();
        watch_clear_indicator(WATCH_INDICATOR_PM);
        watch_clear_indicator(WATCH_INDICATOR_24H);
        watch_display_text(WATCH_POSITION_BOTTOM, zmanim[state->zman_index].label);
    }
}

void zmanim_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(zmanim_state_t));
        memset(*context_ptr, 0, sizeof(zmanim_state_t));
    }
}

void zmanim_face_activate(void *context) {
    if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();

#if __EMSCRIPTEN__
    int16_t browser_lat = EM_ASM_INT({
        return lat;
    });
    int16_t browser_lon = EM_ASM_INT({
        return lon;
    });
    if ((watch_get_backup_data(1) == 0) && (browser_lat || browser_lon)) {
        movement_location_t browser_loc;
        browser_loc.bit.latitude = browser_lat;
        browser_loc.bit.longitude = browser_lon;
        watch_store_backup_data(browser_loc.reg, 1);
    }
#endif

    zmanim_state_t *state = (zmanim_state_t *)context;
    location_settings_load_working_location(&state->location_settings);
}

bool zmanim_face_loop(movement_event_t event, void *context) {
    zmanim_state_t *state = (zmanim_state_t *)context;
    bool location_settings_finished = false;

    if (location_settings_handle_event(&state->location_settings, event, &location_settings_finished)) {
        if (location_settings_finished) _zmanim_face_update(state);
        return true;
    }

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _zmanim_face_update(state);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_TICK:
            if (event.event_type == EVENT_LOW_ENERGY_UPDATE && !watch_sleep_animation_is_running()) watch_start_sleep_animation(1000);
            _zmanim_face_update(state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            if (_zmanim_location_count <= 1) {
                movement_illuminate_led();
            }
            movement_request_tick_frequency(1);
            _zmanim_face_update(state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (_zmanim_location_count <= 1) break;
            else movement_illuminate_led();
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (_zmanim_location_count > 1) {
                state->longLatToUse = (state->longLatToUse + 1) % _zmanim_location_count;
                _zmanim_face_update(state);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->showing_time) state->zman_index = (state->zman_index + 1) % ZMANIM_COUNT;
            state->showing_time = !state->showing_time;
            _zmanim_face_update(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (state->longLatToUse != 0) {
                state->longLatToUse = 0;
                _zmanim_face_update(state);
                break;
            }
            location_settings_begin(&state->location_settings, event);
            break;
        case EVENT_TIMEOUT:
            if (location_settings_load_location().reg == 0) {
                movement_move_to_face(0);
            } else if (state->showing_time || state->zman_index) {
                state->showing_time = false;
                state->zman_index = 0;
                movement_request_tick_frequency(1);
                _zmanim_face_update(state);
            }
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void zmanim_face_resign(void *context) {
    zmanim_state_t *state = (zmanim_state_t *)context;
    state->showing_time = false;
    state->zman_index = 0;
    state->location_settings.page = 0;
    state->location_settings.active_digit = 0;
    location_settings_persist_if_changed(&state->location_settings);
}
