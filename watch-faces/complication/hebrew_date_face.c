/*
 * MIT License
 *
 * Copyright (c) 2026 Rafi
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
#include <stdio.h>
#include "hebrew_date_face.h"
#include "jewish_calendar_utils.h"
#include "watch_rtc.h"
#include "location_settings.h"

/* Fixed Hebrew calendar: 19-year Metonic cycle with the four postponement rules. */
#define HEBREW_EPOCH -1373427L

const char hebrew_date_months_regular[12][HEBREW_DATE_DISPLAY_FIELD_LENGTH] = {
    "Tishre", "Cheshv", "Kislev", "Tevet ", "Shevat", "Adar  ",
    "Nisan ", "Iyar  ", "Sivan ", " TAMUZ", " Av   ", "Elul  "
};

const char hebrew_date_months_leap[13][HEBREW_DATE_DISPLAY_FIELD_LENGTH] = {
    "Tishre", "Cheshv", "Kislev", "Tevet ", "Shevat", "Adar 1", "Adar 2",
    "Nisan ", "Iyar  ", "Sivan ", "TAMUZ", " Av   ", "Elul  "
};

bool hebrew_date_is_leap_year(uint16_t year) {
    uint8_t cycle_year = year % 19;
    return cycle_year == 0 || cycle_year == 3 || cycle_year == 6 || cycle_year == 8 ||
           cycle_year == 11 || cycle_year == 14 || cycle_year == 17;
}

static int32_t hebrew_date_new_year(uint16_t year) {
    int32_t months = (235L * year - 234) / 19;
    int32_t parts = 12084 + 13753 * months;
    int32_t days = 29 * months + parts / 25920;

    if ((3 * (days + 1)) % 7 < 3) days++;
    return HEBREW_EPOCH + days;
}

uint8_t hebrew_date_month_length(uint16_t year, uint8_t month) {
    int32_t year_length = hebrew_date_new_year(year + 1) - hebrew_date_new_year(year);

    if (month == 0) return 30; // Tishrei
    if (month == 1) return year_length % 10 == 5 ? 30 : 29; // Cheshvan
    if (month == 2) return year_length % 10 == 3 ? 29 : 30; // Kislev
    if (month == 3) return 29; // Tevet
    if (month == 4) return 30; // Shevat
    if (hebrew_date_is_leap_year(year)) {
        if (month == 5) return 30; // Adar I
        if (month == 6) return 29; // Adar II
        month -= 7;
    } else {
        if (month == 5) return 29; // Adar
        month -= 6;
    }
    return month % 2 == 0 ? 30 : 29; // Nisan through Elul
}

int32_t hebrew_date_fixed_from_gregorian(uint16_t year, uint8_t month, uint8_t day) {
    uint16_t prior_year = year - 1;
    int32_t fixed = 365L * prior_year + prior_year / 4 - prior_year / 100 + prior_year / 400;

    fixed += (367 * month - 362) / 12 + day;
    if (month > 2) {
        bool leap_year = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
        fixed += leap_year ? -1 : -2;
    }
    return fixed;
}

hebrew_date_t hebrew_date_from_gregorian(uint16_t year, uint8_t month, uint8_t day) {
    int32_t fixed = hebrew_date_fixed_from_gregorian(year, month, day);
    hebrew_date_t hebrew_date = { .year = year + 3760 };

    while (fixed >= hebrew_date_new_year(hebrew_date.year + 1)) hebrew_date.year++;
    while (fixed < hebrew_date_new_year(hebrew_date.year)) hebrew_date.year--;
    hebrew_date.weekday = fixed % 7 + 1;
    fixed -= hebrew_date_new_year(hebrew_date.year);
    while (fixed >= hebrew_date_month_length(hebrew_date.year, hebrew_date.month)) {
        fixed -= hebrew_date_month_length(hebrew_date.year, hebrew_date.month++);
    }
    hebrew_date.day = fixed + 1;

    return hebrew_date;
}

static bool hebrew_date_get_display_date(watch_date_time_t date_time, movement_location_t movement_location, uint16_t *year, uint8_t *month, uint8_t *day, bool *after_sunset_before_alot, int32_t *expires_at) {
    double sunrise, sunset, dawn, night_16_1;
    int16_t current_minute;
    int16_t sunset_minute;
    int16_t dawn_minute;

    if (movement_location.reg == 0) return false;

    *year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR;
    *month = date_time.unit.month;
    *day = date_time.unit.day;

    if (!jewish_calendar_sunrise_sunset_for_date(date_time, movement_location, &sunrise, &sunset)) return false;
    if (!jewish_calendar_solar_time_for_altitude(date_time, movement_location, JEWISH_CALENDAR_ALOT_DEGREES, 0, &dawn, &night_16_1)) return false;

    current_minute = date_time.unit.hour * 60 + date_time.unit.minute;
    sunset_minute = jewish_calendar_local_minute_from_hours(sunset);
    dawn_minute = jewish_calendar_local_minute_from_hours(dawn);
    *after_sunset_before_alot = current_minute >= sunset_minute || current_minute < dawn_minute;

    if (current_minute < dawn_minute) {
        *expires_at = jewish_calendar_fixed_minute(*year, *month, *day, dawn_minute / 60, dawn_minute % 60);
    } else if (current_minute >= sunset_minute) {
        jewish_calendar_advance_gregorian_one_day(year, month, day);
        date_time.unit.year = *year - WATCH_RTC_REFERENCE_YEAR;
        date_time.unit.month = *month;
        date_time.unit.day = *day;
        if (!jewish_calendar_sunrise_sunset_for_date(date_time, movement_location, &sunrise, &sunset)) return false;
        sunset_minute = jewish_calendar_local_minute_from_hours(sunset);
        *expires_at = jewish_calendar_fixed_minute(*year, *month, *day, sunset_minute / 60, sunset_minute % 60);
    } else {
        *expires_at = jewish_calendar_fixed_minute(*year, *month, *day, sunset_minute / 60, sunset_minute % 60);
    }

    return true;
}

static bool hebrew_date_cache_is_current(hebrew_date_state_t *state, watch_date_time_t date_time, movement_location_t movement_location) {
    int32_t now = jewish_calendar_fixed_minute_from_date_time(date_time);

    return state->cache_valid &&
           state->cache_location_reg == movement_location.reg &&
           now >= state->cache_created_at &&
           now < state->cache_expires_at;
}

static void hebrew_date_update_cache(hebrew_date_state_t *state, watch_date_time_t date_time, movement_location_t movement_location) {
    uint16_t gregorian_year;
    uint8_t gregorian_month;
    uint8_t gregorian_day;
    int32_t expires_at;
    bool after_sunset_before_alot = false;

    state->cache_valid = true;
    state->cache_created_at = jewish_calendar_fixed_minute_from_date_time(date_time);
    state->cache_location_reg = movement_location.reg;

    if (!hebrew_date_get_display_date(date_time, movement_location, &gregorian_year, &gregorian_month, &gregorian_day, &after_sunset_before_alot, &expires_at)) {
        state->cached_has_location = false;
        state->cache_expires_at = state->cache_created_at + 60;
        return;
    }

    state->cached_has_location = true;
    state->cached_after_sunset_before_alot = after_sunset_before_alot;
    state->cached_date = hebrew_date_from_gregorian(gregorian_year, gregorian_month, gregorian_day);
    state->cache_expires_at = expires_at;
}

static void hebrew_date_update_display(hebrew_date_state_t *state) {
    watch_date_time_t date_time = movement_get_local_date_time();
    movement_location_t movement_location = location_settings_load_location();
    char day[3];
    char bottom[HEBREW_DATE_DISPLAY_FIELD_LENGTH];
    const char *month_name;

    if (!hebrew_date_cache_is_current(state, date_time, movement_location)) {
        hebrew_date_update_cache(state, date_time, movement_location);
    }

    if (!state->cached_has_location) {
        watch_clear_indicator(WATCH_INDICATOR_PM);
        watch_display_text(WATCH_POSITION_TOP_LEFT, HEBREW_DATE_FACE_LABEL);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, HEBREW_DATE_TOP_RIGHT_BLANK);
        watch_display_text(WATCH_POSITION_BOTTOM, HEBREW_DATE_NO_LOCATION);
        return;
    }

    day[0] = state->cached_date.day < 10 ? ' ' : '0' + (state->cached_date.day / 10);
    day[1] = '0' + (state->cached_date.day % 10);
    day[2] = '\0';
    if (state->show_year) {
        snprintf(bottom, sizeof(bottom), "%6u", state->cached_date.year);
    } else {
        month_name = hebrew_date_is_leap_year(state->cached_date.year) ? hebrew_date_months_leap[state->cached_date.month] : hebrew_date_months_regular[state->cached_date.month];
        memcpy(bottom, month_name, sizeof(bottom));
    }
    watch_display_text(WATCH_POSITION_TOP_LEFT, HEBREW_DATE_FACE_LABEL);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, day);
    watch_display_text(WATCH_POSITION_BOTTOM, bottom);
    if (state->cached_after_sunset_before_alot) watch_set_indicator(WATCH_INDICATOR_PM);
    else watch_clear_indicator(WATCH_INDICATOR_PM);
}

void hebrew_date_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(hebrew_date_state_t));
        memset(*context_ptr, 0, sizeof(hebrew_date_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void hebrew_date_face_activate(void *context) {
    hebrew_date_state_t *state = (hebrew_date_state_t *)context;
    state->show_year = false;
    location_settings_load_working_location(&state->location_settings);
    hebrew_date_update_display(state);
}

bool hebrew_date_face_loop(movement_event_t event, void *context) {
    hebrew_date_state_t *state = (hebrew_date_state_t *)context;
    bool location_settings_finished = false;

    if (location_settings_handle_event(&state->location_settings, event, &location_settings_finished)) {
        if (location_settings_finished) {
            state->cache_valid = false;
            hebrew_date_update_display(state);
        }
        return true;
    }

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            hebrew_date_update_display(state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // You can use the Light button for your own purposes. Note that by default, Movement will also
            // illuminate the LED in response to EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an
            // empty case for EVENT_LIGHT_BUTTON_DOWN.
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            break;
        case EVENT_ALARM_LONG_PRESS:
            location_settings_begin(&state->location_settings, event);
            state->suppress_alarm_hold_in_location_settings = true;
            break;
        case EVENT_ALARM_REALLY_LONG_PRESS:
            break;
        case EVENT_ALARM_LONG_UP:
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->show_year = !state->show_year;
            hebrew_date_update_display(state);
            break;
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            // movement_move_to_face(0);
            break;
        default:
            // Movement's default loop handler will step in for any cases you don't handle above:
            // * EVENT_LIGHT_BUTTON_DOWN lights the LED
            // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
            // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the secondary watch face, if configured)
            // You can override any of these behaviors by adding a case for these events to this switch statement.
            return movement_default_loop_handler(event);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    return true;
}

void hebrew_date_face_resign(void *context) {
    hebrew_date_state_t *state = (hebrew_date_state_t *)context;

    // handle any cleanup before your watch face goes off-screen.
    state->show_year = false;
    state->location_settings.page = 0;
    state->location_settings.active_digit = 0;
    location_settings_persist_if_changed(&state->location_settings);
}
