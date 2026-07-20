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
#include "jewish_calendar_utils.h"
#include "hebrew_date_face.h"
#include "location_settings.h"

typedef struct {
    bool no_tachnun;
    bool day_before;
    char reason[TACHNUN_REASON_LENGTH];
} tachnun_result_t;

static bool _tachnun_is_rosh_chodesh(hebrew_date_t date) {
    return date.day == 1 || date.day == 30;
}

static hebrew_date_t _tachnun_cache_date_and_expiry(watch_date_time_t date_time, movement_location_t movement_location, int32_t *expires_at) {
    uint16_t year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR;
    uint8_t month = date_time.unit.month;
    uint8_t day = date_time.unit.day;
    uint16_t expiry_year = year;
    uint8_t expiry_month = month;
    uint8_t expiry_day = day;
    int16_t dawn_minute;
    int16_t current_minute = date_time.unit.hour * 60 + date_time.unit.minute;

    if (!jewish_calendar_alot_minute_for_date(date_time, movement_location, &dawn_minute)) {
        *expires_at = jewish_calendar_next_local_midnight(date_time);
        return hebrew_date_from_gregorian(year, month, day);
    }

    if (current_minute < dawn_minute) {
        jewish_calendar_retreat_gregorian_one_day(&year, &month, &day);
    } else {
        jewish_calendar_advance_gregorian_one_day(&expiry_year, &expiry_month, &expiry_day);
        date_time.unit.year = expiry_year - WATCH_RTC_REFERENCE_YEAR;
        date_time.unit.month = expiry_month;
        date_time.unit.day = expiry_day;
        if (!jewish_calendar_alot_minute_for_date(date_time, movement_location, &dawn_minute)) {
            dawn_minute = 0;
        }
    }

    *expires_at = jewish_calendar_fixed_minute(expiry_year, expiry_month, expiry_day, dawn_minute / 60, dawn_minute % 60);
    return hebrew_date_from_gregorian(year, month, day);
}

static bool _tachnun_after_mincha_gedolah(void) {
    watch_date_time_t date_time = movement_get_local_date_time();
    movement_location_t movement_location = location_settings_load_location();
    int16_t current_minute;
    int16_t mincha_gedolah_minute;

    if (!jewish_calendar_mincha_gedolah_minute_for_date(date_time, movement_location, &mincha_gedolah_minute)) return false;

    current_minute = date_time.unit.hour * 60 + date_time.unit.minute;
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
            if (date.day <= 2) result = (tachnun_result_t){ true, false, TACHNUN_REASON_ROSH_HASHANA };
            else if (date.day == 9) result = (tachnun_result_t){ true, false, TACHNUN_REASON_EREV_YOM_KIPPUR };
            else if (date.day == 10) result = (tachnun_result_t){ true, false, TACHNUN_REASON_YOM_KIPPUR };
            else if (date.day <= 14) result = (tachnun_result_t){ true, false, TACHNUN_REASON_BETWEEN_HOLIDAYS };
            else if (date.day <= 16) result = (tachnun_result_t){ true, false, TACHNUN_REASON_SUKKOT };
            else if (date.day <= 21) result = (tachnun_result_t){ true, false, TACHNUN_REASON_CHOL_HAMOED };
            else if (date.day == 22) result = (tachnun_result_t){ true, false, TACHNUN_REASON_SHEMINI_ATZERET };
            else if (date.day == 23) result = (tachnun_result_t){ true, false, TACHNUN_REASON_SIMCHAT_TORAH };
            else if (date.day <= 29) result = (tachnun_result_t){ true, false, TACHNUN_REASON_BETWEEN_HOLIDAYS };
            break;
        case 2: // Kislev
            if (date.day >= 25) result = (tachnun_result_t){ true, true, TACHNUN_REASON_CHANUKAH };
            break;
        case 3: // Tevet
            if (date.day <= 2 || (date.day == 3 && hebrew_date_month_length(date.year, 2) == 29)) {
                result = (tachnun_result_t){ true, false, TACHNUN_REASON_CHANUKAH };
            }
            break;
        case 4: // Shevat
            if (date.day == 15) result = (tachnun_result_t){ true, true, TACHNUN_REASON_TU_BISHVAT };
            break;
        case 5: // Adar, or Adar I in leap years
            if (date.day == 14) {
                result = leap ? (tachnun_result_t){ true, true, TACHNUN_REASON_PURIM_KATAN } : (tachnun_result_t){ true, true, TACHNUN_REASON_PURIM };
            } else if (date.day == 15) {
                result = leap ? (tachnun_result_t){ true, false, TACHNUN_REASON_SHUSHAN_PURIM_KATAN } : (tachnun_result_t){ true, false, TACHNUN_REASON_SHUSHAN_PURIM };
            }
            break;
        case 6: // Nisan, or Adar II in leap years
            if (leap) {
                if (date.day == 14) result = (tachnun_result_t){ true, true, TACHNUN_REASON_PURIM };
                else if (date.day == 15) result = (tachnun_result_t){ true, false, TACHNUN_REASON_SHUSHAN_PURIM };
            } else {
                if (date.day >= 2 && date.day <= 14) result = (tachnun_result_t){ true, false, TACHNUN_REASON_NISAN };
                else if (date.day >= 15 && date.day <= 22) result = (tachnun_result_t){ true, true, TACHNUN_REASON_PESACH };
                else if (date.day >= 23 && date.day <= 29) result = (tachnun_result_t){ true, false, TACHNUN_REASON_NISAN };
            }
            break;
        case 7: // Nisan in leap years, Iyyar otherwise
            if (leap) {
                if (date.day >= 2 && date.day <= 14) result = (tachnun_result_t){ true, false, TACHNUN_REASON_NISAN };
                else if (date.day >= 15 && date.day <= 22) result = (tachnun_result_t){ true, true, TACHNUN_REASON_PESACH };
                else if (date.day >= 23 && date.day <= 29) result = (tachnun_result_t){ true, false, TACHNUN_REASON_NISAN };
            } else {
                if (date.day == 14) result = (tachnun_result_t){ true, true, TACHNUN_REASON_PESACH_SHENI };
                else if (date.day == 18) result = (tachnun_result_t){ true, true, TACHNUN_REASON_LAG_BAOMER };
                else if (date.day == 28) result = (tachnun_result_t){ true, true, TACHNUN_REASON_YOM_YERUSHALAYIM };
                else if (_tachnun_is_yom_haatzmaut(date)) result = (tachnun_result_t){ true, true, TACHNUN_REASON_YOM_HAATZMAUT };
            }
            break;
        case 8: // Iyyar in leap years, Sivan otherwise
            if (leap) {
                if (date.day == 14) result = (tachnun_result_t){ true, true, TACHNUN_REASON_PESACH_SHENI };
                else if (date.day == 18) result = (tachnun_result_t){ true, true, TACHNUN_REASON_LAG_BAOMER };
                else if (date.day == 28) result = (tachnun_result_t){ true, true, TACHNUN_REASON_YOM_YERUSHALAYIM };
                else if (_tachnun_is_yom_haatzmaut(date)) result = (tachnun_result_t){ true, true, TACHNUN_REASON_YOM_HAATZMAUT };
            } else {
                if (date.day == 2) result = (tachnun_result_t){ true, false, TACHNUN_REASON_YOM_HAZIKARON };
                else if (date.day >= 3 && date.day <= 5) result = (tachnun_result_t){ true, false, TACHNUN_REASON_HAGBAH };
                else if (date.day >= 6 && date.day <= 7) result = (tachnun_result_t){ true, false, TACHNUN_REASON_SHAVUOT };
                else if (date.day >= 8 && date.day <= 13) result = (tachnun_result_t){ true, false, TACHNUN_REASON_SHAVUOT_WEEK };
            }
            break;
        case 9: // Sivan in leap years, Tammuz otherwise
            if (leap) {
                if (date.day == 2) result = (tachnun_result_t){ true, false, TACHNUN_REASON_YOM_HAZIKARON };
                else if (date.day >= 3 && date.day <= 5) result = (tachnun_result_t){ true, false, TACHNUN_REASON_HAGBAH };
                else if (date.day >= 6 && date.day <= 7) result = (tachnun_result_t){ true, false, TACHNUN_REASON_SHAVUOT };
                else if (date.day >= 8 && date.day <= 13) result = (tachnun_result_t){ true, false, TACHNUN_REASON_SHAVUOT_WEEK };
            }
            break;
        case 10: // Av in regular years, Tammuz in leap years
            if (!leap) {
                if (date.day == 9 || (date.day == 10 && date.weekday == 1)) result = (tachnun_result_t){ true, true, TACHNUN_REASON_TISHA_BAV };
                else if (date.day == 15) result = (tachnun_result_t){ true, true, TACHNUN_REASON_TU_BAV };
            }
            break;
        case 11: // Elul in regular years, Av in leap years
            if (leap) {
                if (date.day == 9 || (date.day == 10 && date.weekday == 1)) result = (tachnun_result_t){ true, true, TACHNUN_REASON_TISHA_BAV };
                else if (date.day == 15) result = (tachnun_result_t){ true, true, TACHNUN_REASON_TU_BAV };
            }
            break;
    }

    if (!result.no_tachnun && date.month == (leap ? 12 : 11) && date.day == 29) {
        result = (tachnun_result_t){ true, false, TACHNUN_REASON_EREV_ROSH_HASHANA };
    }

    if (!result.no_tachnun && _tachnun_is_rosh_chodesh(date)) {
        result = (tachnun_result_t){ true, true, TACHNUN_REASON_ROSH_CHODESH };
    }

    if (!result.no_tachnun && date.weekday == 7) {
        result = (tachnun_result_t){ true, true, TACHNUN_REASON_SHABBOS };
    }

    if (!result.no_tachnun && recurse) {
        tachnun_result_t tomorrow = _tachnun_no_tachnun(jewish_calendar_next_hebrew_day(date), false);
        if (tomorrow.no_tachnun && tomorrow.day_before && _tachnun_after_mincha_gedolah()) {
            result = (tachnun_result_t){ true, false, TACHNUN_REASON_MINCHA };
        }
    }

    return result;
}

static bool _tachnun_cache_is_current(tachnun_state_t *state, watch_date_time_t date_time, movement_location_t movement_location) {
    int32_t now = jewish_calendar_fixed_minute_from_date_time(date_time);

    return state->cache_valid &&
           state->cache_location_reg == movement_location.reg &&
           now >= state->cache_created_at &&
           now < state->cache_expires_at;
}

static void _tachnun_update_cache(tachnun_state_t *state, watch_date_time_t date_time, movement_location_t movement_location) {
    hebrew_date_t date;
    tachnun_result_t result;
    int32_t now = jewish_calendar_fixed_minute_from_date_time(date_time);
    int32_t mincha_at;

    date = _tachnun_cache_date_and_expiry(date_time, movement_location, &state->cache_expires_at);
    result = _tachnun_no_tachnun(date, true);

    if (!result.no_tachnun && jewish_calendar_mincha_gedolah_fixed_minute_for_date(date_time, movement_location, &mincha_at)) {
        tachnun_result_t tomorrow = _tachnun_no_tachnun(jewish_calendar_next_hebrew_day(date), false);
        if (tomorrow.no_tachnun && tomorrow.day_before && now < mincha_at && mincha_at < state->cache_expires_at) {
            state->cache_expires_at = mincha_at;
        }
    }

    state->cache_valid = true;
    state->cache_created_at = now;
    state->cache_location_reg = movement_location.reg;
    state->cached_no_tachnun = result.no_tachnun;
    memcpy(state->cached_reason, result.reason, sizeof(state->cached_reason));
}

static void _tachnun_update(tachnun_state_t *state) {
    watch_date_time_t date_time = movement_get_local_date_time();
    movement_location_t movement_location = location_settings_load_location();

    if (!_tachnun_cache_is_current(state, date_time, movement_location)) {
        _tachnun_update_cache(state, date_time, movement_location);
    }

    watch_display_text(WATCH_POSITION_TOP_LEFT, TACHNUN_FACE_LABEL);
    watch_clear_colon();
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, TACHNUN_TOP_RIGHT_BLANK);

    if (state->showing_reason && state->cached_no_tachnun) {
        watch_display_text(WATCH_POSITION_BOTTOM, state->cached_reason);
    } else {
        watch_display_text(WATCH_POSITION_BOTTOM, state->cached_no_tachnun ? TACHNUN_STATUS_NO : TACHNUN_STATUS_YES);
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
            _tachnun_update(state);
            if (state->cached_no_tachnun) {
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
