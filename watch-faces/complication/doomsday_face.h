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

#pragma once

#include "movement.h"

typedef enum {
    DOOMSDAY_IDLE,
    DOOMSDAY_EDIT_CENTURY,
    DOOMSDAY_EDIT_YEAR,
    DOOMSDAY_EDIT_MONTH,
    DOOMSDAY_EDIT_DAY,
    DOOMSDAY_SHOW_RESULT,
} doomsday_mode_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    doomsday_mode_t mode;
} doomsday_state_t;

void doomsday_face_setup(uint8_t watch_face_index, void ** context_ptr);
void doomsday_face_activate(void *context);
bool doomsday_face_loop(movement_event_t event, void *context);
void doomsday_face_resign(void *context);

#define doomsday_face ((const watch_face_t){ \
    doomsday_face_setup, \
    doomsday_face_activate, \
    doomsday_face_loop, \
    doomsday_face_resign, \
    NULL, \
})
