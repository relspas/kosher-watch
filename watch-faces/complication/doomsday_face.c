/*
 * MIT License
 *
 * Copyright (c) 2026 <#author_name#>
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
#include <stdio.h>
#include <string.h>
#include "doomsday_face.h"
#include "watch.h"
#include "watch_rtc.h"

static bool quick_ticks_running;

static bool _is_leap_year(uint16_t year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static uint8_t _days_in_month(uint16_t year, uint8_t month) {
    static const uint8_t days_in_month[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    if (month == 2 && _is_leap_year(year)) return 29;
    return days_in_month[month - 1];
}

static uint8_t _day_of_week(uint16_t year, uint8_t month, uint8_t day) {
    static const uint8_t month_offsets[] = {
        0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4
    };
    int32_t y = year;
    int32_t weekday;

    if (month < 3) y--;
    weekday = y + y / 4 - y / 100 + y / 400 + month_offsets[month - 1] + day;
    return (weekday % 7 + 7) % 7;
}

static void _normalize_day(doomsday_state_t *state) {
    uint8_t max_day = _days_in_month(state->year, state->month);
    if (state->day > max_day) state->day = max_day;
}

static void _decrement_two_digit_value(uint16_t *value, uint16_t step) {
    uint16_t high = *value / 100;
    uint16_t low = *value % 100;

    low = (low + 100 - step) % 100;
    *value = high * 100 + low;
}

static void _decrement_century(uint16_t *year, uint16_t step) {
    uint16_t century = *year / 100;
    uint16_t year_in_century = *year % 100;

    century = (century + 100 - step) % 100;
    *year = century * 100 + year_in_century;
}

static void _handle_alarm_button(doomsday_state_t *state) {
    switch (state->mode) {
        case DOOMSDAY_IDLE:
            break;
        case DOOMSDAY_EDIT_CENTURY:
            _decrement_century(&state->year, 1);
            _normalize_day(state);
            break;
        case DOOMSDAY_EDIT_YEAR:
            _decrement_two_digit_value(&state->year, 1);
            _normalize_day(state);
            break;
        case DOOMSDAY_EDIT_MONTH:
            state->month = state->month == 1 ? 12 : state->month - 1;
            _normalize_day(state);
            break;
        case DOOMSDAY_EDIT_DAY:
            state->day = state->day == 1 ? _days_in_month(state->year, state->month) : state->day - 1;
            break;
        case DOOMSDAY_SHOW_RESULT:
            state->mode = DOOMSDAY_EDIT_CENTURY;
            break;
    }
}

static void _abort_quick_ticks(void) {
    if (quick_ticks_running) {
        quick_ticks_running = false;
        movement_request_tick_frequency(4);
    }
}

static void _advance_mode(doomsday_state_t *state) {
    switch (state->mode) {
        case DOOMSDAY_IDLE:
            state->mode = DOOMSDAY_EDIT_CENTURY;
            break;
        case DOOMSDAY_EDIT_CENTURY:
            state->mode = DOOMSDAY_EDIT_YEAR;
            break;
        case DOOMSDAY_EDIT_YEAR:
            state->mode = DOOMSDAY_EDIT_MONTH;
            break;
        case DOOMSDAY_EDIT_MONTH:
            state->mode = DOOMSDAY_EDIT_DAY;
            break;
        case DOOMSDAY_EDIT_DAY:
            state->mode = DOOMSDAY_SHOW_RESULT;
            break;
        case DOOMSDAY_SHOW_RESULT:
            state->mode = DOOMSDAY_EDIT_CENTURY;
            break;
    }
}

static void _display(doomsday_state_t *state, uint8_t subsecond) {
    static const char weekdays[7][3] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};
    char buf[7];
    bool blink = subsecond % 2 && !quick_ticks_running;
    uint16_t display_year = state->year % 10000;
    uint8_t display_month = state->month > 12 ? 12 : state->month;

    watch_clear_colon();
    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_PM);
    snprintf(buf, sizeof(buf), "%04u%02u", display_year, display_month);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    snprintf(buf, sizeof(buf), "%2u", state->day);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);

    switch (state->mode) {
        case DOOMSDAY_IDLE:
            watch_display_text(WATCH_POSITION_TOP_LEFT, "dO");
            break;
        case DOOMSDAY_EDIT_CENTURY:
            watch_display_text(WATCH_POSITION_TOP_LEFT, "YE");
            if (blink) watch_display_text(WATCH_POSITION_HOURS, "  ");
            break;
        case DOOMSDAY_EDIT_YEAR:
            watch_display_text(WATCH_POSITION_TOP_LEFT, "YE");
            if (blink) watch_display_text(WATCH_POSITION_MINUTES, "  ");
            break;
        case DOOMSDAY_EDIT_MONTH:
            watch_display_text(WATCH_POSITION_TOP_LEFT, "MO");
            if (blink) watch_display_text(WATCH_POSITION_SECONDS, "  ");
            break;
        case DOOMSDAY_EDIT_DAY:
            watch_display_text(WATCH_POSITION_TOP_LEFT, "dA");
            if (blink) watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
            break;
        case DOOMSDAY_SHOW_RESULT:
            watch_display_text(WATCH_POSITION_TOP_LEFT, weekdays[_day_of_week(state->year, state->month, state->day)]);
            break;
    }
}

void doomsday_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(doomsday_state_t));
        memset(*context_ptr, 0, sizeof(doomsday_state_t));
    }
}

void doomsday_face_activate(void *context) {
    doomsday_state_t *state = (doomsday_state_t *)context;
    watch_date_time_t date_time = movement_get_local_date_time();

    state->year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR;
    state->month = date_time.unit.month;
    state->day = date_time.unit.day;
    state->mode = DOOMSDAY_IDLE;
    quick_ticks_running = false;
    movement_request_tick_frequency(4);
}

bool doomsday_face_loop(movement_event_t event, void *context) {
    doomsday_state_t *state = (doomsday_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            break;
        case EVENT_TICK:
            if (quick_ticks_running) {
                if (HAL_GPIO_BTN_ALARM_read()) _handle_alarm_button(state);
                else _abort_quick_ticks();
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (state->mode != DOOMSDAY_IDLE && state->mode != DOOMSDAY_SHOW_RESULT) {
                quick_ticks_running = true;
                movement_request_tick_frequency(16);
            }
            break;
        case EVENT_ALARM_LONG_UP:
            _abort_quick_ticks();
            break;
        case EVENT_ALARM_BUTTON_UP:
            _abort_quick_ticks();
            _handle_alarm_button(state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            _abort_quick_ticks();
            _advance_mode(state);
            break;
        case EVENT_TIMEOUT:
            _abort_quick_ticks();
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    _display(state, event.subsecond);
    return true;
}

void doomsday_face_resign(void *context) {
    (void) context;
    _abort_quick_ticks();
    movement_request_tick_frequency(1);
}
