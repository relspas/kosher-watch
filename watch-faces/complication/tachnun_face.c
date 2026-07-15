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
#include "tachnun_face.h"
#include "hebrew_date_face.h"
#include "location_settings.h"
#include "sunriset.h"

typedef struct {
    bool no_tachnun;
    bool day_before;
    char reason[7];
} tachnun_result_t;

static hebrew_date_t _tachnun_today(void) {
    watch_date_time_t date_time = movement_get_local_date_time();
    return hebrew_date_from_gregorian(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR, date_time.unit.month, date_time.unit.day);
}

static hebrew_date_t _tachnun_next_day(hebrew_date_t date) {
    date.day++;
    if (date.day > hebrew_date_month_length(date.year, date.month)) {
        date.day = 1;
        date.month++;
        if ((!hebrew_date_is_leap_year(date.year) && date.month > 11) ||
            (hebrew_date_is_leap_year(date.year) && date.month > 12)) {
            date.month = 0;
            date.year++;
        }
    }
    date.weekday = date.weekday == 7 ? 1 : date.weekday + 1;
    return date;
}

static bool _tachnun_is_rosh_chodesh(hebrew_date_t date) {
    return date.day == 1 || date.day == 30;
}

static int16_t _tachnun_local_minute_from_hours(double hours) {
    int16_t minutes = (int16_t)(hours * 60.0 + 0.5);

    while (minutes < 0) minutes += 1440;
    while (minutes >= 1440) minutes -= 1440;

    return minutes;
}

static bool _tachnun_after_mincha_gedolah(void) {
    watch_date_time_t date_time = movement_get_local_date_time();
    movement_location_t movement_location = location_settings_load_location();
    int16_t current_minute;
    int16_t mincha_gedolah_minute;
    double lat, lon, hours_from_utc, sunrise, sunset, gra_day;
    uint16_t year;

    if (movement_location.reg == 0) return false;

    lat = (double)((int16_t)movement_location.bit.latitude) / 100.0;
    lon = (double)((int16_t)movement_location.bit.longitude) / 100.0;
    hours_from_utc = ((double)movement_get_timezone_offset_for_date(date_time)) / 3600.0;
    year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR;

    if (sun_rise_set(year, date_time.unit.month, date_time.unit.day, lon, lat, &sunrise, &sunset) != 0) return false;

    sunrise += hours_from_utc;
    sunset += hours_from_utc;
    gra_day = sunset - sunrise;
    if (gra_day < 0) gra_day += 24.0;

    current_minute = date_time.unit.hour * 60 + date_time.unit.minute;
    mincha_gedolah_minute = _tachnun_local_minute_from_hours(sunrise + gra_day / 2.0 + gra_day / 24.0);

    return current_minute >= mincha_gedolah_minute;
}

static bool _tachnun_is_yom_haatzmaut(hebrew_date_t date) {
    bool leap = hebrew_date_is_leap_year(date.year);
    if (date.month != (leap ? 8 : 7) || date.day < 3 || date.day > 6) return false;

    hebrew_date_t hey_iyyar = date;
    hey_iyyar.day = 5;
    hey_iyyar.weekday = (date.weekday + 7 - ((date.day - 5 + 7) % 7)) % 7;
    if (hey_iyyar.weekday == 0) hey_iyyar.weekday = 7;

    if (hey_iyyar.weekday == 4) return date.day == 5;
    if (hey_iyyar.weekday == 6) return date.day == 4;
    if (hey_iyyar.weekday == 7) return date.day == 3;
    if (hey_iyyar.weekday == 2) return date.day == 6;
    return false;
}

static tachnun_result_t _tachnun_no_tachnun(hebrew_date_t date, bool recurse) {
    tachnun_result_t result = {0};
    bool leap = hebrew_date_is_leap_year(date.year);

    switch (date.month) {
        case 0: // Tishrei
            if (date.day <= 2) result = (tachnun_result_t){ true, false, "RHASH " };
            else if (date.day == 9) result = (tachnun_result_t){ true, false, "EREVYK" };
            else if (date.day == 10) result = (tachnun_result_t){ true, false, "YKIPPR" };
            else if (date.day <= 14) result = (tachnun_result_t){ true, false, "BTWEEN" };
            else if (date.day <= 16) result = (tachnun_result_t){ true, false, "SUKKOT" };
            else if (date.day <= 21) result = (tachnun_result_t){ true, false, "CHOLHM" };
            else if (date.day == 22) result = (tachnun_result_t){ true, false, "SHMINI" };
            else if (date.day == 23) result = (tachnun_result_t){ true, false, "SIMCHT" };
            else if (date.day <= 29) result = (tachnun_result_t){ true, false, "BTWEEN" };
            break;
        case 2: // Kislev
            if (date.day >= 25) result = (tachnun_result_t){ true, true, "CHANUK" };
            break;
        case 3: // Tevet
            if (date.day <= 2 || (date.day == 3 && hebrew_date_month_length(date.year, 2) == 29)) {
                result = (tachnun_result_t){ true, false, "CHANUK" };
            }
            break;
        case 4: // Shevat
            if (date.day == 15) result = (tachnun_result_t){ true, true, "TUBISH" };
            break;
        case 5: // Adar, or Adar I in leap years
            if (date.day == 14) {
                result = leap ? (tachnun_result_t){ true, true, "PURKTN" } : (tachnun_result_t){ true, true, "PURIM " };
            } else if (date.day == 15) {
                result = leap ? (tachnun_result_t){ true, false, "SHSPK " } : (tachnun_result_t){ true, false, "SHUSHP" };
            }
            break;
        case 6: // Nisan, or Adar II in leap years
            if (leap) {
                if (date.day == 14) result = (tachnun_result_t){ true, true, "PURIM " };
                else if (date.day == 15) result = (tachnun_result_t){ true, false, "SHUSHP" };
            } else {
                if (date.day >= 2 && date.day <= 14) result = (tachnun_result_t){ true, false, "NISAN " };
                else if (date.day >= 15 && date.day <= 22) result = (tachnun_result_t){ true, true, "PESACH" };
                else if (date.day >= 23 && date.day <= 29) result = (tachnun_result_t){ true, false, "NISAN " };
            }
            break;
        case 7: // Nisan in leap years, Iyyar otherwise
            if (leap) {
                if (date.day >= 2 && date.day <= 14) result = (tachnun_result_t){ true, false, "NISAN " };
                else if (date.day >= 15 && date.day <= 22) result = (tachnun_result_t){ true, true, "PESACH" };
                else if (date.day >= 23 && date.day <= 29) result = (tachnun_result_t){ true, false, "NISAN " };
            } else {
                if (date.day == 14) result = (tachnun_result_t){ true, true, "PESHNI" };
                else if (date.day == 18) result = (tachnun_result_t){ true, true, "LAGBOM" };
                else if (date.day == 28) result = (tachnun_result_t){ true, true, "YOMYER" };
                else if (_tachnun_is_yom_haatzmaut(date)) result = (tachnun_result_t){ true, true, "YOMHAT" };
            }
            break;
        case 8: // Iyyar in leap years, Sivan otherwise
            if (leap) {
                if (date.day == 14) result = (tachnun_result_t){ true, true, "PESHNI" };
                else if (date.day == 18) result = (tachnun_result_t){ true, true, "LAGBOM" };
                else if (date.day == 28) result = (tachnun_result_t){ true, true, "YOMYER" };
                else if (_tachnun_is_yom_haatzmaut(date)) result = (tachnun_result_t){ true, true, "YOMHAT" };
            } else {
                if (date.day == 2) result = (tachnun_result_t){ true, false, "YOMHMY" };
                else if (date.day >= 3 && date.day <= 5) result = (tachnun_result_t){ true, false, "HAGBAL" };
                else if (date.day >= 6 && date.day <= 7) result = (tachnun_result_t){ true, false, "SHAVOT" };
                else if (date.day >= 8 && date.day <= 13) result = (tachnun_result_t){ true, false, "SHAVWK" };
            }
            break;
        case 9: // Sivan in leap years, Tammuz otherwise
            if (leap) {
                if (date.day == 2) result = (tachnun_result_t){ true, false, "YOMHMY" };
                else if (date.day >= 3 && date.day <= 5) result = (tachnun_result_t){ true, false, "HAGBAL" };
                else if (date.day >= 6 && date.day <= 7) result = (tachnun_result_t){ true, false, "SHAVOT" };
                else if (date.day >= 8 && date.day <= 13) result = (tachnun_result_t){ true, false, "SHAVWK" };
            }
            break;
        case 10: // Av in regular years, Tammuz in leap years
            if (!leap) {
                if (date.day == 9 || (date.day == 10 && date.weekday == 1)) result = (tachnun_result_t){ true, true, "TISHBA" };
                else if (date.day == 15) result = (tachnun_result_t){ true, true, "TUBAV " };
            }
            break;
        case 11: // Elul in regular years, Av in leap years
            if (leap) {
                if (date.day == 9 || (date.day == 10 && date.weekday == 1)) result = (tachnun_result_t){ true, true, "TISHBA" };
                else if (date.day == 15) result = (tachnun_result_t){ true, true, "TUBAV " };
            }
            break;
    }

    if (!result.no_tachnun && date.month == (leap ? 12 : 11) && date.day == 29) {
        result = (tachnun_result_t){ true, false, "EREVRH" };
    }

    if (!result.no_tachnun && _tachnun_is_rosh_chodesh(date)) {
        result = (tachnun_result_t){ true, true, "ROSHCH" };
    }

    if (!result.no_tachnun && date.weekday == 7) {
        result = (tachnun_result_t){ true, true, "SHABOS" };
    }

    if (!result.no_tachnun && recurse) {
        tachnun_result_t tomorrow = _tachnun_no_tachnun(_tachnun_next_day(date), false);
        if (tomorrow.no_tachnun && tomorrow.day_before && _tachnun_after_mincha_gedolah()) {
            result = (tachnun_result_t){ true, false, "MINCHA" };
        }
    }

    return result;
}

static void _tachnun_update(tachnun_state_t *state) {
    tachnun_result_t result = _tachnun_no_tachnun(_tachnun_today(), true);

    watch_display_text(WATCH_POSITION_TOP_LEFT, "TC");
    watch_clear_colon();
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");

    if (state->showing_reason && result.no_tachnun) {
        watch_display_text(WATCH_POSITION_BOTTOM, result.reason);
    } else {
        watch_display_text(WATCH_POSITION_BOTTOM, result.no_tachnun ? " NO   " : " YES  ");
    }
}

void tachnun_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(tachnun_state_t));
        memset(*context_ptr, 0, sizeof(tachnun_state_t));
    }
}

void tachnun_face_activate(void *context) {
    tachnun_state_t *state = (tachnun_state_t *)context;
    state->showing_reason = false;
}

bool tachnun_face_loop(movement_event_t event, void *context) {
    tachnun_state_t *state = (tachnun_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            _tachnun_update(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (_tachnun_no_tachnun(_tachnun_today(), true).no_tachnun) {
                state->showing_reason = !state->showing_reason;
            } else {
                state->showing_reason = false;
            }
            _tachnun_update(state);
            break;
        case EVENT_TIMEOUT:
            state->showing_reason = false;
            _tachnun_update(state);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void tachnun_face_resign(void *context) {
    tachnun_state_t *state = (tachnun_state_t *)context;
    state->showing_reason = false;
}
