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

#include <stdlib.h>
#include <stdio.h>
#include "location_settings.h"
#include "watch_common_display.h"
#include "filesystem.h"

static void _location_settings_persist_location(movement_location_t new_location) {
    movement_location_t maybe_location = {0};

    filesystem_read_file("location.u32", (char *) &maybe_location.reg, sizeof(movement_location_t));
    if (new_location.reg != maybe_location.reg) {
        filesystem_write_file("location.u32", (char *) &new_location.reg, sizeof(movement_location_t));
    }
}

movement_location_t location_settings_load_location(void) {
    movement_location_t location = {0};

    filesystem_read_file("location.u32", (char *) &location.reg, sizeof(movement_location_t));

    return location;
}

static int16_t _location_settings_latlon_from_struct(location_lat_lon_settings_t val) {
    return (val.sign ? -1 : 1) *
        (
            val.hundreds * 10000 +
            val.tens * 1000 +
            val.ones * 100 +
            val.tenths * 10 +
            val.hundredths
        );
}

static location_lat_lon_settings_t _location_settings_struct_from_latlon(int16_t val) {
    location_lat_lon_settings_t retval;

    retval.sign = val < 0;
    val = abs(val);
    retval.hundredths = val % 10;
    val /= 10;
    retval.tenths = val % 10;
    val /= 10;
    retval.ones = val % 10;
    val /= 10;
    retval.tens = val % 10;
    val /= 10;
    retval.hundreds = val % 10;

    return retval;
}

void location_settings_load_working_location(location_settings_state_t *state) {
    movement_location_t movement_location = location_settings_load_location();

    state->working_latitude = _location_settings_struct_from_latlon(movement_location.bit.latitude);
    state->working_longitude = _location_settings_struct_from_latlon(movement_location.bit.longitude);
}

void location_settings_persist_if_changed(location_settings_state_t *state) {
    if (state->location_changed) {
        movement_location_t movement_location = {0};
        movement_location.bit.latitude = _location_settings_latlon_from_struct(state->working_latitude);
        movement_location.bit.longitude = _location_settings_latlon_from_struct(state->working_longitude);
        _location_settings_persist_location(movement_location);
        state->location_changed = false;
    }
}

bool location_settings_is_active(location_settings_state_t *state) {
    return state->page != 0;
}

static void _location_settings_update_display(movement_event_t event, location_settings_state_t *state) {
    char buf[12];

    watch_clear_display();
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_clear_indicator(WATCH_INDICATOR_24H);

    switch (state->page) {
        case 0:
            return;
        case 1:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LAT", "LA");
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                watch_set_decimal_if_available();
                watch_display_character('0' + state->working_latitude.tens, 4);
                watch_display_character('0' + state->working_latitude.ones, 5);
                watch_display_character('0' + state->working_latitude.tenths, 6);
                watch_display_character('0' + state->working_latitude.hundredths, 7);
                watch_display_character('#', 8);
                watch_display_character(state->working_latitude.sign ? 'S' : 'N', 9);

                if (event.subsecond % 2) {
                    watch_display_character(' ', 4 + state->active_digit);
                    if (state->active_digit == 4) watch_display_character(' ', 9);
                }
            } else {
                sprintf(buf, "%c %04d", state->working_latitude.sign ? '-' : '+', abs(_location_settings_latlon_from_struct(state->working_latitude)));
                if (event.subsecond % 2) buf[state->active_digit] = ' ';
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
            }
            break;
        case 2:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LON", "LO");
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                watch_set_decimal_if_available();
                if (state->working_longitude.hundreds == 1) watch_set_pixel(0, 22);
                watch_display_character('0' + state->working_longitude.tens, 4);
                watch_display_character('0' + state->working_longitude.ones, 5);
                watch_display_character('0' + state->working_longitude.tenths, 6);
                watch_display_character('0' + state->working_longitude.hundredths, 7);
                watch_display_character('#', 8);
                watch_display_character(state->working_longitude.sign ? 'W' : 'E', 9);
                if (event.subsecond % 2) {
                    watch_display_character(' ', 4 + state->active_digit);
                    if (state->active_digit == 0) watch_clear_pixel(0, 22);
                    if (state->active_digit == 4) watch_display_character(' ', 9);
                }
            } else {
                sprintf(buf, "%c%05d", state->working_longitude.sign ? '-' : '+', abs(_location_settings_latlon_from_struct(state->working_longitude)));
                if (event.subsecond % 2) buf[state->active_digit] = ' ';
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
            }
            break;
    }
}

static void _location_settings_advance_digit(location_settings_state_t *state) {
    state->location_changed = true;
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        switch (state->page) {
            case 1:
                switch (state->active_digit) {
                    case 0:
                        state->working_latitude.tens = (state->working_latitude.tens + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_latitude)) > 9000) {
                            state->working_latitude.ones = 0;
                            state->working_latitude.tenths = 0;
                            state->working_latitude.hundredths = 0;
                        }
                        break;
                    case 1:
                        state->working_latitude.ones = (state->working_latitude.ones + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.ones = 0;
                        break;
                    case 2:
                        state->working_latitude.tenths = (state->working_latitude.tenths + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.tenths = 0;
                        break;
                    case 3:
                        state->working_latitude.hundredths = (state->working_latitude.hundredths + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.hundredths = 0;
                        break;
                    case 4:
                        state->working_latitude.sign++;
                        break;
                }
                break;
            case 2:
                switch (state->active_digit) {
                    case 0:
                        state->working_longitude.tens++;
                        if (state->working_longitude.tens >= 10) {
                            state->working_longitude.tens = 0;
                            state->working_longitude.hundreds++;
                        }
                        if (abs(_location_settings_latlon_from_struct(state->working_longitude)) > 18000) {
                            state->working_longitude.hundreds = 0;
                            state->working_longitude.tens = 0;
                            state->working_longitude.ones = 0;
                            state->working_longitude.tenths = 0;
                            state->working_longitude.hundredths = 0;
                        }
                        break;
                    case 1:
                        state->working_longitude.ones = (state->working_longitude.ones + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.ones = 0;
                        break;
                    case 2:
                        state->working_longitude.tenths = (state->working_longitude.tenths + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.tenths = 0;
                        break;
                    case 3:
                        state->working_longitude.hundredths = (state->working_longitude.hundredths + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.hundredths = 0;
                        break;
                    case 4:
                        state->working_longitude.sign++;
                        break;
                }
                break;
        }
    } else {
        switch (state->page) {
            case 1:
                switch (state->active_digit) {
                    case 0:
                        state->working_latitude.sign++;
                        break;
                    case 1:
                        break;
                    case 2:
                        state->working_latitude.tens = (state->working_latitude.tens + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_latitude)) > 9000) {
                            state->working_latitude.ones = 0;
                            state->working_latitude.tenths = 0;
                            state->working_latitude.hundredths = 0;
                        }
                        break;
                    case 3:
                        state->working_latitude.ones = (state->working_latitude.ones + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.ones = 0;
                        break;
                    case 4:
                        state->working_latitude.tenths = (state->working_latitude.tenths + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.tenths = 0;
                        break;
                    case 5:
                        state->working_latitude.hundredths = (state->working_latitude.hundredths + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.hundredths = 0;
                        break;
                }
                break;
            case 2:
                switch (state->active_digit) {
                    case 0:
                        state->working_longitude.sign++;
                        break;
                    case 1:
                        state->working_longitude.hundreds = (state->working_longitude.hundreds + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_longitude)) > 18000) {
                            state->working_longitude.tens = 8;
                            state->working_longitude.ones = 0;
                            state->working_longitude.tenths = 0;
                            state->working_longitude.hundredths = 0;
                        }
                        break;
                    case 2:
                        state->working_longitude.tens = (state->working_longitude.tens + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.tens = 0;
                        break;
                    case 3:
                        state->working_longitude.ones = (state->working_longitude.ones + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.ones = 0;
                        break;
                    case 4:
                        state->working_longitude.tenths = (state->working_longitude.tenths + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.tenths = 0;
                        break;
                    case 5:
                        state->working_longitude.hundredths = (state->working_longitude.hundredths + 1) % 10;
                        if (abs(_location_settings_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.hundredths = 0;
                        break;
                }
                break;
        }
    }
}

void location_settings_begin(location_settings_state_t *state, movement_event_t event) {
    location_settings_load_working_location(state);
    state->page = 1;
    state->active_digit = 0;
    watch_clear_display();
    movement_request_tick_frequency(4);
    _location_settings_update_display(event, state);
}

static void _location_settings_finish(location_settings_state_t *state) {
    state->active_digit = 0;
    state->page = 0;
    location_settings_persist_if_changed(state);
    movement_request_tick_frequency(1);
}

bool location_settings_handle_event(location_settings_state_t *state, movement_event_t event, bool *finished) {
    *finished = false;

    if (!location_settings_is_active(state)) return false;

    switch (event.event_type) {
        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_TICK:
            _location_settings_update_display(event, state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                state->active_digit++;
                if (state->active_digit > 4) {
                    state->active_digit = 0;
                    state->page = (state->page + 1) % 3;
                    location_settings_persist_if_changed(state);
                }
            } else {
                state->active_digit++;
                if (state->page == 1 && state->active_digit == 1) state->active_digit++;
                if (state->active_digit > 5) {
                    state->active_digit = 0;
                    state->page = (state->page + 1) % 3;
                    location_settings_persist_if_changed(state);
                }
            }
            if (state->page == 0) {
                _location_settings_finish(state);
                *finished = true;
            } else {
                _location_settings_update_display(event, state);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            _location_settings_advance_digit(state);
            _location_settings_update_display(event, state);
            break;
        case EVENT_ALARM_LONG_PRESS:
        case EVENT_TIMEOUT:
            _location_settings_finish(state);
            *finished = true;
            break;
        default:
            return false;
    }

    return true;
}
