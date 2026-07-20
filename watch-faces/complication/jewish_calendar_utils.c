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

#include "jewish_calendar_utils.h"
#include "watch_rtc.h"
#include "sunriset.h"

uint8_t jewish_calendar_gregorian_month_length(uint16_t year, uint8_t month) {
    static const uint8_t month_lengths[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    if (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) return 29;

    return month_lengths[month - 1];
}

void jewish_calendar_advance_gregorian_one_day(uint16_t *year, uint8_t *month, uint8_t *day) {
    (*day)++;
    if (*day <= jewish_calendar_gregorian_month_length(*year, *month)) return;

    *day = 1;
    (*month)++;
    if (*month <= 12) return;

    *month = 1;
    (*year)++;
}

void jewish_calendar_retreat_gregorian_one_day(uint16_t *year, uint8_t *month, uint8_t *day) {
    if (*day > 1) {
        (*day)--;
        return;
    }

    if (*month > 1) {
        (*month)--;
    } else {
        *month = 12;
        (*year)--;
    }
    *day = jewish_calendar_gregorian_month_length(*year, *month);
}

hebrew_date_t jewish_calendar_next_hebrew_day(hebrew_date_t date) {
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

int32_t jewish_calendar_fixed_minute(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute) {
    return hebrew_date_fixed_from_gregorian(year, month, day) * 1440L + hour * 60L + minute;
}

int32_t jewish_calendar_fixed_minute_from_date_time(watch_date_time_t date_time) {
    return jewish_calendar_fixed_minute(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR,
                                        date_time.unit.month,
                                        date_time.unit.day,
                                        date_time.unit.hour,
                                        date_time.unit.minute);
}

int32_t jewish_calendar_next_local_midnight(watch_date_time_t date_time) {
    int32_t today = hebrew_date_fixed_from_gregorian(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR,
                                                     date_time.unit.month,
                                                     date_time.unit.day);

    return (today + 1) * 1440L;
}

int16_t jewish_calendar_local_minute_from_hours(double hours) {
    int16_t minutes;

    while (hours < 0) hours += 24.0;
    while (hours >= 24.0) hours -= 24.0;

    minutes = (int16_t)(hours * 60.0 + 0.5);
    while (minutes >= 1440) minutes -= 1440;

    return minutes;
}

int32_t jewish_calendar_fixed_minute_from_local_hours(uint16_t year, uint8_t month, uint8_t day, double hours) {
    int32_t fixed = hebrew_date_fixed_from_gregorian(year, month, day);

    while (hours < 0) {
        hours += 24.0;
        fixed--;
    }
    while (hours >= 24.0) {
        hours -= 24.0;
        fixed++;
    }

    return fixed * 1440L + jewish_calendar_local_minute_from_hours(hours);
}

bool jewish_calendar_location_to_degrees(movement_location_t movement_location, double *latitude, double *longitude) {
    if (movement_location.reg == 0) return false;

    *latitude = (double)((int16_t)movement_location.bit.latitude) / 100.0;
    *longitude = (double)((int16_t)movement_location.bit.longitude) / 100.0;
    return true;
}

bool jewish_calendar_solar_time_for_altitude(watch_date_time_t date_time, movement_location_t movement_location, double altitude, int upper_limb, double *rise, double *set) {
    double latitude;
    double longitude;
    double hours_from_utc;
    uint16_t year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR;

    if (!jewish_calendar_location_to_degrees(movement_location, &latitude, &longitude)) return false;

    hours_from_utc = ((double)movement_get_timezone_offset_for_date(date_time)) / 3600.0;
    if (__sunriset__(year, date_time.unit.month, date_time.unit.day, longitude, latitude, altitude, upper_limb, rise, set) != 0) return false;

    *rise += hours_from_utc;
    *set += hours_from_utc;
    return true;
}

bool jewish_calendar_sunrise_sunset_for_date(watch_date_time_t date_time, movement_location_t movement_location, double *sunrise, double *sunset) {
    return jewish_calendar_solar_time_for_altitude(date_time, movement_location, -35.0 / 60.0, 1, sunrise, sunset);
}

bool jewish_calendar_alot_minute_for_date(watch_date_time_t date_time, movement_location_t movement_location, int16_t *alot_minute) {
    double dawn;
    double night_16_1;

    if (!jewish_calendar_solar_time_for_altitude(date_time, movement_location, JEWISH_CALENDAR_ALOT_DEGREES, 0, &dawn, &night_16_1)) return false;

    *alot_minute = jewish_calendar_local_minute_from_hours(dawn);
    return true;
}

bool jewish_calendar_mincha_gedolah_minute_for_date(watch_date_time_t date_time, movement_location_t movement_location, int16_t *mincha_gedolah_minute) {
    double sunrise;
    double sunset;
    double gra_day;

    if (!jewish_calendar_sunrise_sunset_for_date(date_time, movement_location, &sunrise, &sunset)) return false;

    gra_day = sunset - sunrise;
    if (gra_day < 0) gra_day += 24.0;

    *mincha_gedolah_minute = jewish_calendar_local_minute_from_hours(sunrise + gra_day / 2.0 + gra_day / 24.0);
    return true;
}

bool jewish_calendar_mincha_gedolah_fixed_minute_for_date(watch_date_time_t date_time, movement_location_t movement_location, int32_t *mincha_gedolah_at) {
    int16_t mincha_gedolah_minute;

    if (!jewish_calendar_mincha_gedolah_minute_for_date(date_time, movement_location, &mincha_gedolah_minute)) return false;

    *mincha_gedolah_at = jewish_calendar_fixed_minute(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR,
                                                      date_time.unit.month,
                                                      date_time.unit.day,
                                                      mincha_gedolah_minute / 60,
                                                      mincha_gedolah_minute % 60);
    return true;
}
