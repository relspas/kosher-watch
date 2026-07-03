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
#include "watch_rtc.h"

/* Fixed Hebrew calendar: 19-year Metonic cycle with the four postponement rules. */
#define HEBREW_EPOCH -1373427L

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

static void hebrew_date_update_display(hebrew_date_state_t *state) {
    static const char *const months_regular[] = {
        "Tishri", "Cheshv", "Kislev", "Tevet ", "Shevat", "Adar  ",
        "Nisan ", "Iyar  ", "Sivan ", " TAMUZ", "Av    ", "Elul  "
    };
    static const char *const months_leap[] = {
        "Tishri", "Cheshv", "Kislev", "Tevet ", "Shevat", "Adar 1", "Adar 2",
        "Nisan ", "Iyar  ", "Sivan ", "Tamuz ", "Av    ", "Elul  "
    };
    watch_date_time_t date_time = movement_get_local_date_time();
    uint16_t gregorian_year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR;
    hebrew_date_t hebrew_date = hebrew_date_from_gregorian(gregorian_year, date_time.unit.month, date_time.unit.day);
    char day[3];
    char bottom[7];

    day[0] = hebrew_date.day < 10 ? ' ' : '0' + (hebrew_date.day / 10);
    day[1] = '0' + (hebrew_date.day % 10);
    day[2] = '\0';
    if (state->show_year) {
        snprintf(bottom, sizeof(bottom), "  %4u", hebrew_date.year);
    } else {
        snprintf(bottom, sizeof(bottom), "%s",
                 hebrew_date_is_leap_year(hebrew_date.year) ? months_leap[hebrew_date.month] : months_regular[hebrew_date.month]);
    }
    watch_display_text(WATCH_POSITION_TOP_LEFT, "HE");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, day);
    watch_display_text(WATCH_POSITION_BOTTOM, bottom);
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
    hebrew_date_update_display(state);
}

bool hebrew_date_face_loop(movement_event_t event, void *context) {
    hebrew_date_state_t *state = (hebrew_date_state_t *)context;

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
        case EVENT_ALARM_REALLY_LONG_PRESS:
            state->show_year = true;
            hebrew_date_update_display(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
        case EVENT_ALARM_LONG_UP:
            state->show_year = false;
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
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}
