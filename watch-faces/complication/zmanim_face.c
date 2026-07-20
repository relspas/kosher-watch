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
#include "zmanim_face.h"
#include "jewish_calendar_utils.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_common_display.h"
#include "location_settings.h"
#include "hebrew_date_face.h"

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define ZMANIM_TALIS_DEGREES -10.2
#define ZMANIM_NIGHTFALL_3_STARS_DEGREES -8.5

const char zmanim_base_labels[ZMANIM_BASE_COUNT][ZMANIM_DISPLAY_FIELD_LENGTH] = {
    ZMANIM_LABEL_ALOT,
    ZMANIM_LABEL_TALIS,
    ZMANIM_LABEL_NETZ,
    ZMANIM_LABEL_MAGEN_AVRAHAM_SHEMA,
    ZMANIM_LABEL_GRA_SHEMA,
    ZMANIM_LABEL_SHACHARIT,
    ZMANIM_LABEL_CHATZOT,
    ZMANIM_LABEL_MINCHA_GEDOLAH,
    ZMANIM_LABEL_PLAG,
    ZMANIM_LABEL_SHKIA,
    ZMANIM_LABEL_THREE_STARS,
    ZMANIM_LABEL_SEVENTY_TWO_MINUTES
};

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

static uint8_t _zmanim_adar_for_purim(uint16_t year) {
    return hebrew_date_is_leap_year(year) ? 6 : 5;
}

static uint8_t _zmanim_nisan(uint16_t year) {
    return hebrew_date_is_leap_year(year) ? 7 : 6;
}

static uint8_t _zmanim_sivan(uint16_t year) {
    return hebrew_date_is_leap_year(year) ? 9 : 8;
}

static uint8_t _zmanim_tammuz(uint16_t year) {
    return hebrew_date_is_leap_year(year) ? 10 : 9;
}

static uint8_t _zmanim_av(uint16_t year) {
    return hebrew_date_is_leap_year(year) ? 11 : 10;
}

static bool _zmanim_is_taanit_esther(hebrew_date_t date) {
    uint8_t adar = _zmanim_adar_for_purim(date.year);

    if (date.month != adar) return false;
    if (date.day == 13 && date.weekday != 7) return true;
    return date.day == 11 && date.weekday == 5;
}

static bool _zmanim_is_tzom_gedalia(hebrew_date_t date) {
    if (date.month != 0) return false;
    if (date.day == 3 && date.weekday != 7) return true;
    return date.day == 4 && date.weekday == 1;
}

static bool _zmanim_is_seventeenth_tammuz(hebrew_date_t date) {
    if (date.month != _zmanim_tammuz(date.year)) return false;
    if (date.day == 17 && date.weekday != 7) return true;
    return date.day == 18 && date.weekday == 1;
}

static bool _zmanim_is_tisha_bav_fast_day(hebrew_date_t date) {
    if (date.month != _zmanim_av(date.year)) return false;
    if (date.day == 9 && date.weekday != 7) return true;
    return date.day == 10 && date.weekday == 1;
}

static bool _zmanim_is_daytime_public_fast(hebrew_date_t date) {
    if (_zmanim_is_tzom_gedalia(date)) return true;
    if (date.month == 3 && date.day == 10) return true;
    if (_zmanim_is_taanit_esther(date)) return true;
    return _zmanim_is_seventeenth_tammuz(date);
}

static bool _zmanim_is_yom_kippur(hebrew_date_t date) {
    return date.month == 0 && date.day == 10;
}

static bool _zmanim_is_erev_yom_kippur(hebrew_date_t date) {
    return date.month == 0 && date.day == 9;
}

static bool _zmanim_is_erev_tisha_bav(hebrew_date_t date) {
    return _zmanim_is_tisha_bav_fast_day(jewish_calendar_next_hebrew_day(date));
}

static bool _zmanim_is_first_day_holiday(hebrew_date_t date) {
    uint8_t nisan = _zmanim_nisan(date.year);
    uint8_t sivan = _zmanim_sivan(date.year);

    if (date.month == 0) {
        return date.day == 1 || date.day == 10 || date.day == 15 || date.day == 22;
    }
    if (date.month == nisan) return date.day == 15 || date.day == 21;
    if (date.month == sivan) return date.day == 6;
    return false;
}

static bool _zmanim_is_erev_candle_day(hebrew_date_t date) {
    return date.weekday == 6 || _zmanim_is_first_day_holiday(jewish_calendar_next_hebrew_day(date));
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

static void _zmanim_add_label(zmanim_entry_t *zmanim, uint8_t *count, const char *label) {
    if (*count >= ZMANIM_DISPLAY_COUNT_MAX) return;

    memcpy(zmanim[*count].label, label, sizeof(zmanim[*count].label));
    zmanim[*count].minute = 0;
    zmanim[*count].is_time = false;
    (*count)++;
}

static void _zmanim_add_time(zmanim_entry_t *zmanim, uint8_t *count, int16_t minute) {
    if (*count >= ZMANIM_DISPLAY_COUNT_MAX) return;

    memset(zmanim[*count].label, 0, sizeof(zmanim[*count].label));
    zmanim[*count].minute = minute;
    zmanim[*count].is_time = true;
    (*count)++;
}

static bool _zmanim_compute_base(zmanim_state_t *state, movement_location_t movement_location, zmanim_entry_t *zmanim) {
    (void) state;
    if (movement_location.reg == 0) return false;

    watch_date_time_t date_time = movement_get_local_date_time();
    double sunrise, sunset, dawn, night_16_1, talis, night_3_stars_start, night_3_stars;

    if (!jewish_calendar_sunrise_sunset_for_date(date_time, movement_location, &sunrise, &sunset)) return false;
    if (!jewish_calendar_solar_time_for_altitude(date_time, movement_location, JEWISH_CALENDAR_ALOT_DEGREES, 0, &dawn, &night_16_1)) return false;
    if (!jewish_calendar_solar_time_for_altitude(date_time, movement_location, ZMANIM_TALIS_DEGREES, 0, &talis, &night_3_stars_start)) return false;
    if (!jewish_calendar_solar_time_for_altitude(date_time, movement_location, ZMANIM_NIGHTFALL_3_STARS_DEGREES, 0, &night_3_stars_start, &night_3_stars)) return false;

    double gra_day = sunset - sunrise;
    double magen_avraham_day = night_16_1 - dawn;
    int16_t minutes[ZMANIM_BASE_COUNT];

    if (gra_day < 0) gra_day += 24.0;
    if (magen_avraham_day < 0) magen_avraham_day += 24.0;

    minutes[0] = jewish_calendar_local_minute_from_hours(dawn);
    minutes[1] = jewish_calendar_local_minute_from_hours(talis);
    minutes[2] = jewish_calendar_local_minute_from_hours(sunrise);
    minutes[3] = jewish_calendar_local_minute_from_hours(dawn + magen_avraham_day / 4.0);
    minutes[4] = jewish_calendar_local_minute_from_hours(sunrise + gra_day / 4.0);
    minutes[5] = jewish_calendar_local_minute_from_hours(sunrise + gra_day / 3.0);
    minutes[6] = jewish_calendar_local_minute_from_hours(sunrise + gra_day / 2.0);
    minutes[7] = jewish_calendar_local_minute_from_hours(sunrise + gra_day / 2.0 + gra_day / 24.0);
    minutes[8] = jewish_calendar_local_minute_from_hours(sunset - gra_day * 1.25 / 12.0);
    minutes[9] = jewish_calendar_local_minute_from_hours(sunset);
    minutes[10] = jewish_calendar_local_minute_from_hours(night_3_stars);
    minutes[11] = jewish_calendar_local_minute_from_hours(sunset + 1.2);

    for (uint8_t i = 0; i < ZMANIM_BASE_COUNT; i++) {
        memcpy(zmanim[i].label, zmanim_base_labels[i], sizeof(zmanim[i].label));
        zmanim[i].minute = minutes[i];
        zmanim[i].is_time = true;
    }

    return true;
}

static bool _zmanim_compute(zmanim_state_t *state, movement_location_t movement_location, zmanim_entry_t *zmanim, uint8_t *count) {
    zmanim_entry_t base_zmanim[ZMANIM_BASE_COUNT];
    watch_date_time_t date_time = movement_get_local_date_time();
    hebrew_date_t today = hebrew_date_from_gregorian(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR,
                                                     date_time.unit.month,
                                                     date_time.unit.day);
    bool daytime_fast = _zmanim_is_daytime_public_fast(today);
    bool tisha_bav = _zmanim_is_tisha_bav_fast_day(today);
    bool yom_kippur = _zmanim_is_yom_kippur(today);
    bool erev_tisha_bav = _zmanim_is_erev_tisha_bav(today);
    bool erev_yom_kippur = _zmanim_is_erev_yom_kippur(today);
    bool candle_day = _zmanim_is_erev_candle_day(today);
    int16_t candle_minute;

    *count = 0;
    if (!_zmanim_compute_base(state, movement_location, base_zmanim)) return false;

    candle_minute = base_zmanim[9].minute - 18;
    if (candle_minute < 0) candle_minute += 1440;

    for (uint8_t i = 0; i < ZMANIM_BASE_COUNT; i++) {
        _zmanim_add_label(zmanim, count, base_zmanim[i].label);
        if (i == 0 && daytime_fast) _zmanim_add_label(zmanim, count, ZMANIM_LABEL_BEGIN_FAST);
        if (i == 10 && (daytime_fast || tisha_bav || yom_kippur)) _zmanim_add_label(zmanim, count, ZMANIM_LABEL_END_FAST);
        _zmanim_add_time(zmanim, count, base_zmanim[i].minute);

        if (i == 8) {
            if (erev_yom_kippur) {
                _zmanim_add_label(zmanim, count, ZMANIM_LABEL_BEGIN_FAST);
                _zmanim_add_label(zmanim, count, ZMANIM_LABEL_CANDLE);
                _zmanim_add_time(zmanim, count, candle_minute);
            } else {
                if (erev_tisha_bav) {
                    _zmanim_add_label(zmanim, count, ZMANIM_LABEL_BEGIN_FAST);
                    _zmanim_add_time(zmanim, count, candle_minute);
                }
                if (candle_day) {
                    _zmanim_add_label(zmanim, count, ZMANIM_LABEL_CANDLE);
                    _zmanim_add_time(zmanim, count, candle_minute);
                }
            }
        }
    }

    return *count > 0;
}

static bool _zmanim_halakhic_midnight_expiry(watch_date_time_t date_time, movement_location_t movement_location, int32_t *expires_at) {
    uint16_t year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR;
    uint8_t month = date_time.unit.month;
    uint8_t day = date_time.unit.day;
    uint16_t next_year = year;
    uint8_t next_month = month;
    uint8_t next_day = day;
    double sunrise, sunset, next_sunrise, next_sunset;

    if (movement_location.reg == 0) {
        *expires_at = jewish_calendar_next_local_midnight(date_time);
        return true;
    }

    jewish_calendar_advance_gregorian_one_day(&next_year, &next_month, &next_day);
    watch_date_time_t next_date_time = date_time;
    next_date_time.unit.year = next_year - WATCH_RTC_REFERENCE_YEAR;
    next_date_time.unit.month = next_month;
    next_date_time.unit.day = next_day;

    if (!jewish_calendar_sunrise_sunset_for_date(date_time, movement_location, &sunrise, &sunset)) return false;
    if (!jewish_calendar_sunrise_sunset_for_date(next_date_time, movement_location, &next_sunrise, &next_sunset)) return false;

    if (next_sunrise < sunset) next_sunrise += 24.0;

    *expires_at = jewish_calendar_fixed_minute_from_local_hours(year, month, day, sunset + (next_sunrise - sunset) / 2.0);
    return true;
}

static bool _zmanim_cache_is_current(zmanim_state_t *state, watch_date_time_t date_time, movement_location_t movement_location) {
    int32_t now = jewish_calendar_fixed_minute_from_date_time(date_time);

    return state->cache_valid &&
           state->cache_location_reg == movement_location.reg &&
           now >= state->cache_created_at &&
           now < state->cache_expires_at;
}

static void _zmanim_update_cache(zmanim_state_t *state, watch_date_time_t date_time, movement_location_t movement_location) {
    state->cache_valid = true;
    state->cache_created_at = jewish_calendar_fixed_minute_from_date_time(date_time);
    state->cache_location_reg = movement_location.reg;

    if (!_zmanim_compute(state, movement_location, state->cached_zmanim, &state->cached_zmanim_count)) {
        state->cached_has_location = false;
        state->cached_zmanim_count = 0;
        state->cache_expires_at = jewish_calendar_next_local_midnight(date_time);
        return;
    }

    state->cached_has_location = true;
    if (!_zmanim_halakhic_midnight_expiry(date_time, movement_location, &state->cache_expires_at)) {
        state->cache_expires_at = state->cache_created_at + 60;
    }
}

static bool _zmanim_ensure_cache(zmanim_state_t *state) {
    watch_date_time_t date_time = movement_get_local_date_time();
    movement_location_t movement_location = _zmanim_get_location(state);

    if (!_zmanim_cache_is_current(state, date_time, movement_location)) {
        _zmanim_update_cache(state, date_time, movement_location);
    }

    if (state->zman_index >= state->cached_zmanim_count) state->zman_index = 0;
    return state->cached_has_location && state->cached_zmanim_count > 0;
}

static void _zmanim_face_update(zmanim_state_t *state) {
    char buf[7];
    zmanim_entry_t *entry;

    watch_clear_colon();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, ZMANIM_FACE_LABEL, ZMANIM_FACE_LABEL);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, ZMANIM_TOP_RIGHT_BLANK);

    _zmanim_ensure_cache(state);

    if (!state->cached_has_location || state->cached_zmanim_count == 0) {
        watch_clear_indicator(WATCH_INDICATOR_PM);
        watch_clear_indicator(WATCH_INDICATOR_24H);
        watch_display_text(WATCH_POSITION_BOTTOM, ZMANIM_NO_LOCATION);
        return;
    }

    entry = &state->cached_zmanim[state->zman_index];
    state->showing_time = entry->is_time;

    if (entry->is_time) {
        watch_set_colon();
        _zmanim_format_time(buf, entry->minute);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        watch_clear_colon();
        watch_clear_indicator(WATCH_INDICATOR_PM);
        watch_clear_indicator(WATCH_INDICATOR_24H);
        watch_display_text(WATCH_POSITION_BOTTOM, entry->label);
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
        if (location_settings_finished) {
            state->cache_valid = false;
            _zmanim_face_update(state);
        }
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
                state->cache_valid = false;
                _zmanim_face_update(state);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (_zmanim_ensure_cache(state)) {
                state->zman_index = (state->zman_index + 1) % state->cached_zmanim_count;
            } else {
                state->zman_index = 0;
            }
            _zmanim_face_update(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (state->longLatToUse != 0) {
                state->longLatToUse = 0;
                state->cache_valid = false;
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
