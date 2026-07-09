/*
 * MIT License
 *
 * Copyright (c) 2026
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
    uint8_t sign: 1;
    uint8_t hundreds: 5;
    uint8_t tens: 5;
    uint8_t ones: 4;
    uint8_t tenths: 4;
    uint8_t hundredths: 4;
} location_lat_lon_settings_t;

typedef struct {
    uint8_t page;
    uint8_t active_digit;
    bool location_changed;
    location_lat_lon_settings_t working_latitude;
    location_lat_lon_settings_t working_longitude;
} location_settings_state_t;

movement_location_t location_settings_load_location(void);
void location_settings_load_working_location(location_settings_state_t *state);
void location_settings_persist_if_changed(location_settings_state_t *state);
bool location_settings_is_active(location_settings_state_t *state);
void location_settings_begin(location_settings_state_t *state, movement_event_t event);
bool location_settings_handle_event(location_settings_state_t *state, movement_event_t event, bool *finished);
