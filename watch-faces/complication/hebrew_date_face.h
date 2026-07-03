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

typedef struct {
    uint16_t year;
    uint8_t month; // 0=Tishrei, 1=Cheshvan ... Adar/Nisan positions follow year type.
    uint8_t day;
    uint8_t weekday; // 1=Sunday ... 7=Shabbat.
} hebrew_date_t;

typedef struct {
    bool show_year;
} hebrew_date_state_t;

void hebrew_date_face_setup(uint8_t watch_face_index, void ** context_ptr);
void hebrew_date_face_activate(void *context);
bool hebrew_date_face_loop(movement_event_t event, void *context);
void hebrew_date_face_resign(void *context);

bool hebrew_date_is_leap_year(uint16_t year);
uint8_t hebrew_date_month_length(uint16_t year, uint8_t month);
int32_t hebrew_date_fixed_from_gregorian(uint16_t year, uint8_t month, uint8_t day);
hebrew_date_t hebrew_date_from_gregorian(uint16_t year, uint8_t month, uint8_t day);

#define hebrew_date_face ((const watch_face_t){ \
    hebrew_date_face_setup, \
    hebrew_date_face_activate, \
    hebrew_date_face_loop, \
    hebrew_date_face_resign, \
    NULL, \
})
