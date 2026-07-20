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

#define DAF_YOMI_FACE_LABEL "DF"
#define DAF_YOMI_TOP_RIGHT_HUNDREDS " 1"
#define DAF_YOMI_TOP_RIGHT_BLANK "  "
#define DAF_YOMI_NO_DAF "------"
#define DAF_YOMI_MASECHTOT 40
#define DAF_YOMI_DISPLAY_FIELD_LENGTH 7

/*
 * A DESCRIPTION OF YOUR WATCH FACE
 *
 * and a description of how use it
 *
 */

typedef struct {
    char text[DAF_YOMI_DISPLAY_FIELD_LENGTH];
    bool special;
} daf_yomi_pair_t;

extern const daf_yomi_pair_t daf_yomi_pairs[DAF_YOMI_MASECHTOT];

typedef struct {
    uint8_t daf;
    uint8_t mesechet;
    bool cache_valid;
    bool cached_has_daf;
    int32_t cache_created_at;
    int32_t cache_expires_at;
} daf_yomi_state_t;

void daf_yomi_face_setup(uint8_t watch_face_index, void ** context_ptr);
void daf_yomi_face_activate(void *context);
bool daf_yomi_face_loop(movement_event_t event, void *context);
void daf_yomi_face_resign(void *context);

#define daf_yomi_face ((const watch_face_t){ \
    daf_yomi_face_setup, \
    daf_yomi_face_activate, \
    daf_yomi_face_loop, \
    daf_yomi_face_resign, \
    NULL, \
})
