/*
 * Host-side tests for the Tachnun face logic in tachnun_face.c.
 *
 * Example build from the repository root:
 *   gcc -std=c99 -Wall -Wextra \
 *     -Iwatch-faces/test/include \
 *     -Iwatch-faces/complication \
 *     watch-faces/test/tachnun_test.c \
 *     -o /tmp/tachnun_test
 *
 * Example run:
 *   /tmp/tachnun_test
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "movement.h"
#include "watch_rtc.h"
#include "location_settings.h"

void watch_clear_colon(void);
void watch_set_colon(void);
void watch_set_pixel(uint8_t com, uint8_t seg);
void watch_clear_pixel(uint8_t com, uint8_t seg);
void watch_display_character(uint8_t character, uint8_t position);
void watch_display_text_with_fallback(watch_position_t location, const char *string, const char *fallback);
void watch_set_decimal_if_available(void);
void location_settings_load_working_location(location_settings_state_t *state);
void location_settings_persist_if_changed(location_settings_state_t *state);
bool location_settings_is_active(location_settings_state_t *state);
void location_settings_begin(location_settings_state_t *state, movement_event_t event);
bool location_settings_handle_event(location_settings_state_t *state, movement_event_t event, bool *finished);

#include "../complication/hebrew_date_face.c"
#include "../complication/tachnun_face.c"

static int failures = 0;
static watch_date_time_t fake_now;
static movement_location_t fake_location;
static char displayed_bottom[8];

#define ASSERT_EQ_STR(expected, actual) assert_eq_str((expected), (actual), #actual, __LINE__)

static void assert_eq_str(const char *expected, const char *actual, const char *expr, int line) {
    if (strcmp(expected, actual) != 0) {
        printf("line %d: expected %s to be \"%s\", got \"%s\"\n",
               line, expr, expected, actual);
        failures++;
    }
}

static void reset_face_test_state(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, bool has_location) {
    memset(&fake_now, 0, sizeof(fake_now));
    fake_now.unit.year = year - WATCH_RTC_REFERENCE_YEAR;
    fake_now.unit.month = month;
    fake_now.unit.day = day;
    fake_now.unit.hour = hour;
    fake_now.unit.minute = minute;

    memset(&fake_location, 0, sizeof(fake_location));
    if (has_location) {
        fake_location.bit.latitude = 4072;
        fake_location.bit.longitude = -7401;
    }

    memset(displayed_bottom, 0, sizeof(displayed_bottom));
}

watch_date_time_t movement_get_local_date_time(void) {
    return fake_now;
}

int32_t movement_get_timezone_offset_for_date(watch_date_time_t date_time) {
    (void)date_time;
    return 0;
}

movement_location_t location_settings_load_location(void) {
    return fake_location;
}

void watch_display_text(watch_position_t location, const char *string) {
    if (location == WATCH_POSITION_BOTTOM) {
        snprintf(displayed_bottom, sizeof(displayed_bottom), "%s", string);
    }
}

void watch_set_indicator(watch_indicator_t indicator) {
    (void)indicator;
}

void watch_clear_indicator(watch_indicator_t indicator) {
    (void)indicator;
}

void watch_clear_colon(void) {
}

void watch_set_colon(void) {
}

bool movement_default_loop_handler(movement_event_t event) {
    (void)event;
    return true;
}

void movement_request_tick_frequency(uint8_t freq) {
    (void)freq;
}

void location_settings_load_working_location(location_settings_state_t *state) {
    (void)state;
}

void location_settings_persist_if_changed(location_settings_state_t *state) {
    (void)state;
}

bool location_settings_is_active(location_settings_state_t *state) {
    (void)state;
    return false;
}

void location_settings_begin(location_settings_state_t *state, movement_event_t event) {
    (void)state;
    (void)event;
}

bool location_settings_handle_event(location_settings_state_t *state, movement_event_t event, bool *finished) {
    (void)state;
    (void)event;
    (void)finished;
    return false;
}

void watch_set_pixel(uint8_t com, uint8_t seg) {
    (void)com;
    (void)seg;
}

void watch_clear_pixel(uint8_t com, uint8_t seg) {
    (void)com;
    (void)seg;
}

void watch_display_character(uint8_t character, uint8_t position) {
    (void)character;
    (void)position;
}

void watch_display_text_with_fallback(watch_position_t location, const char *string, const char *fallback) {
    (void)fallback;
    watch_display_text(location, string);
}

void watch_set_decimal_if_available(void) {
}

int __sunriset__(int year, int month, int day, double lon, double lat,
                 double altit, int upper_limb, double *rise, double *set) {
    (void)year;
    (void)month;
    (void)day;
    (void)lon;
    (void)lat;
    (void)altit;
    (void)upper_limb;
    *rise = 6.0;
    *set = 18.0;
    return 0;
}

static void update_tachnun_display(bool showing_reason) {
    tachnun_state_t state = { .showing_reason = showing_reason };

    tachnun_face_loop((movement_event_t){ .event_type = EVENT_TICK }, &state);
}

static void test_day_before_no_tachnun_starts_at_mincha_gedolah(void) {
    reset_face_test_state(2025, 2, 12, 12, 29, true);
    update_tachnun_display(false);
    ASSERT_EQ_STR(" YES  ", displayed_bottom);

    reset_face_test_state(2025, 2, 12, 12, 30, true);
    update_tachnun_display(false);
    ASSERT_EQ_STR(" NO   ", displayed_bottom);

    update_tachnun_display(true);
    ASSERT_EQ_STR("MINCHA", displayed_bottom);
}

static void test_day_before_no_tachnun_requires_location_for_mincha_time(void) {
    reset_face_test_state(2025, 2, 12, 13, 0, false);
    update_tachnun_display(false);
    ASSERT_EQ_STR(" YES  ", displayed_bottom);
}

int main(void) {
    test_day_before_no_tachnun_starts_at_mincha_gedolah();
    test_day_before_no_tachnun_requires_location_for_mincha_time();

    if (failures) {
        printf("%d Tachnun test failure%s\n", failures, failures == 1 ? "" : "s");
        return 1;
    }

    printf("Tachnun tests passed\n");
    return 0;
}
