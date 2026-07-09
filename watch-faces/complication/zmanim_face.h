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
#include "location_settings.h"

/*
 * ZMANIM FACE
 *
 * Shows common Jewish times for the current civil day. Press Alarm to move
 * from a zman name to its time, and press Alarm again for the next zman.
 * Long-press Alarm to set latitude and longitude, using the same location
 * register as the Sunrise/Sunset face.
 */

typedef struct {
    uint8_t zman_index;
    bool showing_time;
    uint8_t longLatToUse;
    location_settings_state_t location_settings;
} zmanim_state_t;

void zmanim_face_setup(uint8_t watch_face_index, void ** context_ptr);
void zmanim_face_activate(void *context);
bool zmanim_face_loop(movement_event_t event, void *context);
void zmanim_face_resign(void *context);

#define zmanim_face ((const watch_face_t){ \
    zmanim_face_setup, \
    zmanim_face_activate, \
    zmanim_face_loop, \
    zmanim_face_resign, \
    NULL, \
})

typedef struct {
    char name[3];
    int16_t latitude;
    int16_t longitude;
} zmanim_long_lat_presets_t;

static const zmanim_long_lat_presets_t zmanimLongLatPresets[] =
{
    { .name = "  "},  // Default: use the shared location register.
//    { .name = "Ny", .latitude = 4072, .longitude = -7401 },  // New York City, NY
//    { .name = "LA", .latitude = 3405, .longitude = -11824 },  // Los Angeles, CA
//    { .name = "dE", .latitude = 4221, .longitude = -8305 },  // Detroit, MI
};
