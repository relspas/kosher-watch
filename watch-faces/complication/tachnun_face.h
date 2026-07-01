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

/*
 * TACHNUN FACE
 *
 * Shows whether Tachanun is said today according to the conventions used by
 * IsThereTachanunToday. Press Alarm to show the reason when Tachanun is not
 * said.
 */

typedef struct {
    bool showing_reason;
} tachnun_state_t;

void tachnun_face_setup(uint8_t watch_face_index, void ** context_ptr);
void tachnun_face_activate(void *context);
bool tachnun_face_loop(movement_event_t event, void *context);
void tachnun_face_resign(void *context);

#define tachnun_face ((const watch_face_t){ \
    tachnun_face_setup, \
    tachnun_face_activate, \
    tachnun_face_loop, \
    tachnun_face_resign, \
    NULL, \
})
