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

#pragma once

#include "movement.h"
#include "hebrew_date_face.h"

#define JEWISH_CALENDAR_ALOT_DEGREES -16.1

uint8_t jewish_calendar_gregorian_month_length(uint16_t year, uint8_t month);
void jewish_calendar_advance_gregorian_one_day(uint16_t *year, uint8_t *month, uint8_t *day);
void jewish_calendar_retreat_gregorian_one_day(uint16_t *year, uint8_t *month, uint8_t *day);

hebrew_date_t jewish_calendar_next_hebrew_day(hebrew_date_t date);

int32_t jewish_calendar_fixed_minute(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute);
int32_t jewish_calendar_fixed_minute_from_date_time(watch_date_time_t date_time);
int32_t jewish_calendar_next_local_midnight(watch_date_time_t date_time);
int32_t jewish_calendar_fixed_minute_from_local_hours(uint16_t year, uint8_t month, uint8_t day, double hours);
int16_t jewish_calendar_local_minute_from_hours(double hours);

bool jewish_calendar_location_to_degrees(movement_location_t movement_location, double *latitude, double *longitude);
bool jewish_calendar_sunrise_sunset_for_date(watch_date_time_t date_time, movement_location_t movement_location, double *sunrise, double *sunset);
bool jewish_calendar_solar_time_for_altitude(watch_date_time_t date_time, movement_location_t movement_location, double altitude, int upper_limb, double *rise, double *set);
bool jewish_calendar_alot_minute_for_date(watch_date_time_t date_time, movement_location_t movement_location, int16_t *alot_minute);
bool jewish_calendar_mincha_gedolah_minute_for_date(watch_date_time_t date_time, movement_location_t movement_location, int16_t *mincha_gedolah_minute);
bool jewish_calendar_mincha_gedolah_fixed_minute_for_date(watch_date_time_t date_time, movement_location_t movement_location, int32_t *mincha_gedolah_at);
