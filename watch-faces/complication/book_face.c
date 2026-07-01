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
#include "book_face.h"

// static const char book_text[] = "abcdefghijklmnopqrstuvwxyz";
static const char book_text[] = "I wish it was Saturday instead of Sunday. Any day you have to take a bath and to go to bed early isn't a day off in my book. Autumn Sundays are the worst. You can ever really enjoy Sunday's because in the back of your mind you know you have to go to school the next day. And when the leaves change color it just reminds you even more that summers over and school's just begun. Gee, I like this season best of all! The trees are like natures own fireworks display! I love the brisk air, the early evenings, the ... uh ... the ... yes, well ... hmm. Kapow! Fwoosh! Zingg! You didn't mention fresh applesauce, fuzzhead. Do you like applesauce.";

static void book_face_display_text(book_state_t *state) {
    char window[7];

    memcpy(window, &book_text[state->text_offset], 6);
    window[6] = '\0';
    watch_display_text(WATCH_POSITION_BOTTOM, window);
}

static void book_face_display_words(book_state_t *state) {
    char window[7];
    uint16_t text_index = state->text_offset;
    uint8_t window_index = 0;

    memset(window, ' ', 6);
    while (book_text[text_index] == ' ') text_index++;
    while (book_text[text_index] != '\0') {
        uint16_t word_start = text_index;
        uint8_t word_length = 0;

        while (book_text[text_index] != ' ' && book_text[text_index] != '\0') {
            text_index++;
            word_length++;
        }
        if (window_index && window_index + 1 + word_length > 6) break;
        if (word_length > 6) word_length = 6;
        if (window_index) window[window_index++] = ' ';
        memcpy(&window[window_index], &book_text[word_start], word_length);
        window_index += word_length;
        while (book_text[text_index] == ' ') text_index++;
    }
    window[6] = '\0';
    watch_display_text(WATCH_POSITION_BOTTOM, window);
}

static void book_face_advance_word(book_state_t *state) {
    uint16_t text_index = state->text_offset;
    uint8_t window_index = 0;

    while (book_text[text_index] == ' ') text_index++;
    while (book_text[text_index] != '\0') {
        uint8_t word_length = 0;

        while (book_text[text_index + word_length] != ' ' && book_text[text_index + word_length] != '\0') word_length++;
        if (window_index && window_index + 1 + word_length > 6) break;
        text_index += word_length;
        window_index += word_length + (window_index ? 1 : 0);
        while (book_text[text_index] == ' ') text_index++;
    }
    state->text_offset = book_text[text_index] == '\0' ? 0 : text_index;
}

static void book_face_display(book_state_t *state) {
    if (state->word_mode) {
        book_face_display_words(state);
    } else {
        book_face_display_text(state);
    }
}

void book_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(book_state_t));
        memset(*context_ptr, 0, sizeof(book_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void book_face_activate(void *context) {
    book_state_t *state = (book_state_t *)context;

    state->text_offset = 0;
    state->word_mode = false;
    book_face_display(state);
    movement_request_tick_frequency(4);
}

bool book_face_loop(movement_event_t event, void *context) {
    book_state_t *state = (book_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            book_face_activate(context);
            break;
        case EVENT_TICK:
            if (state->word_mode) {
                book_face_advance_word(state);
            } else {
                state->text_offset++;
                if (state->text_offset > strlen(book_text) - 6) {
                    state->text_offset = 0;
                }
            }
            book_face_display(state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // You can use the Light button for your own purposes. Note that by default, Movement will also
            // illuminate the LED in response to EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an
            // empty case for EVENT_LIGHT_BUTTON_DOWN.
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->word_mode = !state->word_mode;
            state->text_offset = 0;
            book_face_display(state);
            break;
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            // movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // If you did not resign in EVENT_TIMEOUT, you can use this event to update the display once a minute.
            // Avoid displaying fast-updating values like seconds, since the display won't update again for 60 seconds.
            // You should also consider starting the tick animation, to show the wearer that this is sleep mode:
            // watch_start_sleep_animation(500);
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

void book_face_resign(void *context) {
    (void) context;

    movement_request_tick_frequency(1);
    // handle any cleanup before your watch face goes off-screen.
}
