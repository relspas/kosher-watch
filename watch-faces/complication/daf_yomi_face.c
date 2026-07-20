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

#include <stdlib.h>
#include <string.h>
#include "daf_yomi_face.h"
#include "jewish_calendar_utils.h"
#include "watch_rtc.h"

#define DAF_YOMI_START_YEAR 1923
#define DAF_YOMI_START_MONTH 9
#define DAF_YOMI_START_DAY 11
#define DAF_YOMI_SHEKALIM_CHANGE_YEAR 1975
#define DAF_YOMI_SHEKALIM_CHANGE_MONTH 6
#define DAF_YOMI_SHEKALIM_CHANGE_DAY 24
#define DAF_YOMI_OLD_CYCLE_DAYS 2702
#define DAF_YOMI_CYCLE_DAYS 2711

static void daf_yomi_update_display(daf_yomi_state_t *state);

void daf_yomi_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(daf_yomi_state_t));
        memset(*context_ptr, 0, sizeof(daf_yomi_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void daf_yomi_face_activate(void *context) {
    daf_yomi_update_display((daf_yomi_state_t *)context);
}

const daf_yomi_pair_t daf_yomi_pairs[DAF_YOMI_MASECHTOT] = {
    { "BrAC", false },
    { "Shab", false },
    { "Eruv", false },
    { "eESA", false },
    { "Shek", false },
    { "4oMA", false },
    { "Suca", false },
    { "BezA", false },
    { "Rosh", false },
    { "Taan", false }, // " tANi%1d"
    { "ME GIL", true }, // use M at top
    { "Mo  Ed", true },
    { "Chag", false },
    { "4EvA", false },
    { "Ketu", false },
    { "NEdr", false },
    { "NAzr", false },
    { "Sota", false },
    { "Gitt", false },
    { "Kidush", true },
    { "BABA K", true },
    { "BABA M", true },
    { "BABA b", true },
    { "Sanh", false },
    { "Ma cot", true }, // use M at top
    { "Shev", false },
    { "AV OdA", true },
    { "HorA", false },
    { "Zeva", false },
    { "ME NAC", true }, // m at top
    { "ChUL", false },
    { "BEch", false },
    { "Arac", false },
    { "mute r", true }, //use M at top
    { "Krit", false }, //or " kri"
    { "ME ilA", true }, //M at top
    { "Kinn", false },
    { "Tami", false },
    { "Midd", false },
    { "MIta d", true }, //M at top
};

static const uint8_t daf_yomi_daf_counts[DAF_YOMI_MASECHTOT] = {
    64, 157, 105, 121, 22, 88, 56, 40, 35, 31, 32, 29, 27, 122, 112, 91,
    66, 49, 90, 82, 119, 119, 176, 113, 24, 49, 76, 14, 120, 110, 142,
    61, 34, 34, 28, 22, 4, 9, 5, 73
};

static bool daf_yomi_get_today(daf_yomi_state_t *state) {
    watch_date_time_t date_time = movement_get_local_date_time();
    uint16_t year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR;
    int32_t today = hebrew_date_fixed_from_gregorian(year, date_time.unit.month, date_time.unit.day);
    int32_t start = hebrew_date_fixed_from_gregorian(DAF_YOMI_START_YEAR, DAF_YOMI_START_MONTH, DAF_YOMI_START_DAY);
    int32_t shekalim_change = hebrew_date_fixed_from_gregorian(DAF_YOMI_SHEKALIM_CHANGE_YEAR,
                                                               DAF_YOMI_SHEKALIM_CHANGE_MONTH,
                                                               DAF_YOMI_SHEKALIM_CHANGE_DAY);
    uint8_t counts[DAF_YOMI_MASECHTOT];
    int32_t day_in_cycle;

    if (today < start) return false;
    memcpy(counts, daf_yomi_daf_counts, sizeof(counts));
    if (today < shekalim_change) {
        day_in_cycle = (today - start) % DAF_YOMI_OLD_CYCLE_DAYS;
        counts[4] = 13;
    } else {
        day_in_cycle = (today - shekalim_change) % DAF_YOMI_CYCLE_DAYS;
    }

    for (uint8_t mesechet = 0; mesechet < DAF_YOMI_MASECHTOT; mesechet++) {
        uint8_t days_in_mesechet = counts[mesechet] - 1;
        if (day_in_cycle < days_in_mesechet) {
            state->mesechet = mesechet;
            state->daf = day_in_cycle + 2;
            if (mesechet == 36) state->daf += 21; // Kinnim pagination
            if (mesechet == 37) state->daf += 24; // Tamid pagination
            if (mesechet == 38) state->daf += 32; // Middot pagination
            return true;
        }
        day_in_cycle -= days_in_mesechet;
    }
    return false;
}

static bool daf_yomi_cache_is_current(daf_yomi_state_t *state, watch_date_time_t date_time) {
    int32_t now = jewish_calendar_fixed_minute_from_date_time(date_time);

    return state->cache_valid &&
           now >= state->cache_created_at &&
           now < state->cache_expires_at;
}

static void daf_yomi_update_cache(daf_yomi_state_t *state, watch_date_time_t date_time) {
    state->cache_valid = true;
    state->cache_created_at = jewish_calendar_fixed_minute_from_date_time(date_time);
    state->cache_expires_at = jewish_calendar_next_local_midnight(date_time);
    state->cached_has_daf = daf_yomi_get_today(state);
}

static void _display_mesechet_daf(daf_yomi_state_t *state){
    char buf[11];
    uint8_t daf = state->daf;
    if (!daf_yomi_pairs[state->mesechet].special){
        watch_display_text(WATCH_POSITION_TOP_LEFT, DAF_YOMI_FACE_LABEL);
        sprintf(buf,"%.4s%2d", daf_yomi_pairs[state->mesechet].text, daf % 100);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }else{
        sprintf(buf,"%c%c%c%c%2d", daf_yomi_pairs[state->mesechet].text[2],
            daf_yomi_pairs[state->mesechet].text[3],daf_yomi_pairs[state->mesechet].text[4],
            daf_yomi_pairs[state->mesechet].text[5], daf % 100);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
        sprintf(buf,"%.2s",daf_yomi_pairs[state->mesechet].text);
        watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
    }
    if (daf / 100 > 0)
        watch_display_text(WATCH_POSITION_TOP_RIGHT, DAF_YOMI_TOP_RIGHT_HUNDREDS);
    else
        watch_display_text(WATCH_POSITION_TOP_RIGHT, DAF_YOMI_TOP_RIGHT_BLANK);
}

static void daf_yomi_update_display(daf_yomi_state_t *state) {
    watch_date_time_t date_time = movement_get_local_date_time();

    if (!daf_yomi_cache_is_current(state, date_time)) {
        daf_yomi_update_cache(state, date_time);
    }

    if (state->cached_has_daf) {
        _display_mesechet_daf(state);
    } else {
        watch_display_text(WATCH_POSITION_TOP_LEFT, DAF_YOMI_FACE_LABEL);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, DAF_YOMI_TOP_RIGHT_BLANK);
        watch_display_text(WATCH_POSITION_BOTTOM, DAF_YOMI_NO_DAF);
    }
}

bool daf_yomi_face_loop(movement_event_t event, void *context) {
    daf_yomi_state_t *state = (daf_yomi_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            daf_yomi_update_display(state);
            break;
        case EVENT_TICK:
            daf_yomi_update_display(state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // You can use the Light button for your own purposes. Note that by default, Movement will also
            // illuminate the LED in response to EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an
            // empty case for EVENT_LIGHT_BUTTON_DOWN.
            break;
        case EVENT_ALARM_BUTTON_UP:
            daf_yomi_update_display(state);
            break;
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            // movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            daf_yomi_update_display(state);
            break;
        default:
            // Movement's default loop handler will step in for any cases you don't handle above:
            // * EVENT_LIGHT_BUTTON_DOWN lights the LED
            // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
            // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the secondary watch face, if configured)
            // You can override any of these behaviors by adding a case for these events to this switch statement.
            return movement_default_loop_handler(event);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    return true;
}

void daf_yomi_face_resign(void *context) {
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}
