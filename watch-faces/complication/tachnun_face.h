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

#define TACHNUN_FACE_LABEL "TC"
#define TACHNUN_TOP_RIGHT_BLANK "  "
#define TACHNUN_STATUS_NO " NO   "
#define TACHNUN_STATUS_YES " YES  "
#define TACHNUN_REASON_LENGTH 7

#define TACHNUN_REASON_ROSH_HASHANA "RHASH "
#define TACHNUN_REASON_EREV_YOM_KIPPUR "EREVYK"
#define TACHNUN_REASON_YOM_KIPPUR "YKIPPR"
#define TACHNUN_REASON_BETWEEN_HOLIDAYS "BTWEEN"
#define TACHNUN_REASON_SUKKOT "SUKKOT"
#define TACHNUN_REASON_CHOL_HAMOED "CHOLHM"
#define TACHNUN_REASON_SHEMINI_ATZERET "SHMINI"
#define TACHNUN_REASON_SIMCHAT_TORAH "SIMCHT"
#define TACHNUN_REASON_CHANUKAH "CHANUK"
#define TACHNUN_REASON_TU_BISHVAT "TUBISH"
#define TACHNUN_REASON_PURIM_KATAN "PURKTN"
#define TACHNUN_REASON_PURIM "PURIM "
#define TACHNUN_REASON_SHUSHAN_PURIM_KATAN "SHSPK "
#define TACHNUN_REASON_SHUSHAN_PURIM "SHUSHP"
#define TACHNUN_REASON_NISAN "NISAN "
#define TACHNUN_REASON_PESACH "PESACH"
#define TACHNUN_REASON_PESACH_SHENI "PESHNI"
#define TACHNUN_REASON_LAG_BAOMER "LAGBOM"
#define TACHNUN_REASON_YOM_YERUSHALAYIM "YOMYER"
#define TACHNUN_REASON_YOM_HAATZMAUT "YOMHAT"
#define TACHNUN_REASON_YOM_HAZIKARON "YOMHMY"
#define TACHNUN_REASON_HAGBAH "HAGBAL"
#define TACHNUN_REASON_SHAVUOT "SHAVOT"
#define TACHNUN_REASON_SHAVUOT_WEEK "SHAVWK"
#define TACHNUN_REASON_TISHA_BAV "TISHBA"
#define TACHNUN_REASON_TU_BAV "TUBAV "
#define TACHNUN_REASON_EREV_ROSH_HASHANA "EREVRH"
#define TACHNUN_REASON_ROSH_CHODESH "ROSHCH"
#define TACHNUN_REASON_SHABBOS "SHABOS"
#define TACHNUN_REASON_MINCHA "MINCHA"

/*
 * TACHNUN FACE
 *
 * Shows whether Tachanun is said today according to the conventions used by
 * IsThereTachanunToday. Press Alarm to show the reason when Tachanun is not
 * said.
 */

typedef struct {
    bool showing_reason;
    bool cache_valid;
    bool cached_no_tachnun;
    int32_t cache_created_at;
    int32_t cache_expires_at;
    uint32_t cache_location_reg;
    char cached_reason[TACHNUN_REASON_LENGTH];
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
