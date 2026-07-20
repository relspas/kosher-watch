/*
 * Host-side tests for the Zmanim face logic in zmanim_face.c.
 *
 * Example build from the repository root:
 *   gcc -std=c99 -Wall -Wextra \
 *     -Iwatch-faces/test/include \
 *     -Iwatch-faces/complication \
 *     watch-faces/test/zmanim_test.c \
 *     -lm \
 *     -o /tmp/zmanim_test
 *
 * Example run:
 *   /tmp/zmanim_test
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "movement.h"
#include "watch_rtc.h"
#include "location_settings.h"

void watch_clear_colon(void);
void watch_set_colon(void);
void watch_display_text_with_fallback(watch_position_t location, const char *string, const char *fallback);
void watch_clear_indicator(watch_indicator_t indicator);
void watch_set_indicator(watch_indicator_t indicator);
bool movement_clock_mode_24h(void);
bool watch_utility_convert_to_12_hour(watch_date_time_t *date_time);
bool watch_sleep_animation_is_running(void);
void watch_stop_sleep_animation(void);
void watch_start_sleep_animation(uint32_t duration_ms);
void movement_illuminate_led(void);
void movement_move_to_face(uint8_t watch_face_index);
uint32_t watch_get_backup_data(uint8_t reg);
void watch_store_backup_data(uint32_t data, uint8_t reg);
void location_settings_load_working_location(location_settings_state_t *state);
void location_settings_persist_if_changed(location_settings_state_t *state);
bool location_settings_handle_event(location_settings_state_t *state, movement_event_t event, bool *finished);
void location_settings_begin(location_settings_state_t *state, movement_event_t event);
movement_location_t location_settings_load_location(void);

#include "../complication/hebrew_date_face.c"
#include "../complication/jewish_calendar_utils.c"
#include "../complication/zmanim_face.c"

static int failures = 0;
static watch_date_time_t fake_now;
static movement_location_t fake_location;
static char displayed_bottom[8];
static bool fake_colon;
static const char *current_test_name;

typedef struct {
    const char *name;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    int16_t latitude;
    int16_t longitude;
    uint8_t zman_index;
    const char *expected;
    double dawn;
    double night_16_1;
    double talis;
    double sunrise;
    double sunset;
    double night_3_stars;
} zman_time_case_t;

static const zman_time_case_t *active_zman_case;

#define ASSERT_EQ_STR(expected, actual) assert_eq_str((expected), (actual), #actual, __LINE__)
#define ASSERT_TRUE(actual) assert_true((actual), #actual, __LINE__)

static void assert_eq_str(const char *expected, const char *actual, const char *expr, int line) {
    if (strcmp(expected, actual) != 0) {
        printf("%s line %d: expected %s to be \"%s\", got \"%s\"\n",
               current_test_name, line, expr, expected, actual);
        failures++;
    }
}

static void assert_true(bool actual, const char *expr, int line) {
    if (!actual) {
        printf("%s line %d: expected %s to be true\n", current_test_name, line, expr);
        failures++;
    }
}

static zman_time_case_t default_zman_case(const char *name, uint8_t zman_index, const char *expected) {
    return (zman_time_case_t) {
        .name = name,
        .year = 2026,
        .month = 7,
        .day = 20,
        .hour = 12,
        .minute = 0,
        .latitude = 4072,
        .longitude = -7401,
        .zman_index = zman_index,
        .expected = expected,
        .dawn = 5.0,
        .night_16_1 = 19.0,
        .talis = 5.5,
        .sunrise = 6.0,
        .sunset = 18.0,
        .night_3_stars = 18.75,
    };
}

static void reset_zmanim_test_state(const zman_time_case_t *test_case) {
    memset(&fake_now, 0, sizeof(fake_now));
    fake_now.unit.year = test_case->year - WATCH_RTC_REFERENCE_YEAR;
    fake_now.unit.month = test_case->month;
    fake_now.unit.day = test_case->day;
    fake_now.unit.hour = test_case->hour;
    fake_now.unit.minute = test_case->minute;

    memset(&fake_location, 0, sizeof(fake_location));
    fake_location.bit.latitude = test_case->latitude;
    fake_location.bit.longitude = test_case->longitude;

    memset(displayed_bottom, 0, sizeof(displayed_bottom));
    fake_colon = false;
    active_zman_case = test_case;
    current_test_name = test_case->name;
}

static void assert_zman_time(const zman_time_case_t *test_case) {
    zmanim_state_t state = {0};
    uint8_t time_index = 0;
    bool found = false;

    reset_zmanim_test_state(test_case);
    ASSERT_TRUE(_zmanim_ensure_cache(&state));

    for (uint8_t i = 0; i < state.cached_zmanim_count; i++) {
        if (!state.cached_zmanim[i].is_time) continue;
        if (time_index == test_case->zman_index) {
            state.zman_index = i;
            found = true;
            break;
        }
        time_index++;
    }

    ASSERT_TRUE(found);
    _zmanim_face_update(&state);

    ASSERT_EQ_STR(test_case->expected, displayed_bottom);
    ASSERT_TRUE(fake_colon);
}

static void assert_zman_display_at(const zman_time_case_t *test_case, uint8_t display_index, const char *expected, bool expected_colon) {
    zmanim_state_t state = {0};

    reset_zmanim_test_state(test_case);
    ASSERT_TRUE(_zmanim_ensure_cache(&state));
    if (display_index >= state.cached_zmanim_count) {
        printf("%s: display index %u out of range, count is %u\n",
               current_test_name, display_index, state.cached_zmanim_count);
        failures++;
        return;
    }

    state.zman_index = display_index;
    _zmanim_face_update(&state);

    ASSERT_EQ_STR(expected, displayed_bottom);
    if (expected_colon) ASSERT_TRUE(fake_colon);
    else if (fake_colon) {
        printf("%s: expected colon to be clear at display index %u\n",
               current_test_name, display_index);
        failures++;
    }
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

void watch_display_text_with_fallback(watch_position_t location, const char *string, const char *fallback) {
    (void)fallback;
    watch_display_text(location, string);
}

void watch_clear_indicator(watch_indicator_t indicator) {
    (void)indicator;
}

void watch_set_indicator(watch_indicator_t indicator) {
    (void)indicator;
}

void watch_clear_colon(void) {
    fake_colon = false;
}

void watch_set_colon(void) {
    fake_colon = true;
}

bool movement_clock_mode_24h(void) {
    return true;
}

bool watch_utility_convert_to_12_hour(watch_date_time_t *date_time) {
    (void)date_time;
    return false;
}

bool watch_sleep_animation_is_running(void) {
    return false;
}

void watch_stop_sleep_animation(void) {
}

void watch_start_sleep_animation(uint32_t duration_ms) {
    (void)duration_ms;
}

void movement_illuminate_led(void) {
}

void movement_move_to_face(uint8_t watch_face_index) {
    (void)watch_face_index;
}

uint32_t watch_get_backup_data(uint8_t reg) {
    (void)reg;
    return 0;
}

void watch_store_backup_data(uint32_t data, uint8_t reg) {
    (void)data;
    (void)reg;
}

void movement_request_tick_frequency(uint8_t freq) {
    (void)freq;
}

bool movement_default_loop_handler(movement_event_t event) {
    (void)event;
    return true;
}

void location_settings_load_working_location(location_settings_state_t *state) {
    (void)state;
}

void location_settings_persist_if_changed(location_settings_state_t *state) {
    (void)state;
}

bool location_settings_handle_event(location_settings_state_t *state, movement_event_t event, bool *finished) {
    (void)state;
    (void)event;
    *finished = false;
    return false;
}

void location_settings_begin(location_settings_state_t *state, movement_event_t event) {
    (void)state;
    (void)event;
}

int __sunriset__(int year, int month, int day, double lon, double lat,
                 double altit, int upper_limb, double *rise, double *set) {
    (void)year;
    (void)month;
    (void)day;
    (void)lon;
    (void)lat;
    (void)upper_limb;

    if (active_zman_case == NULL) return -1;

    if (fabs(altit - (-35.0 / 60.0)) < 0.001) {
        *rise = active_zman_case->sunrise;
        *set = active_zman_case->sunset;
    } else if (fabs(altit - JEWISH_CALENDAR_ALOT_DEGREES) < 0.001) {
        *rise = active_zman_case->dawn;
        *set = active_zman_case->night_16_1;
    } else if (fabs(altit - ZMANIM_TALIS_DEGREES) < 0.001) {
        *rise = active_zman_case->talis;
        *set = active_zman_case->night_3_stars;
    } else if (fabs(altit - ZMANIM_NIGHTFALL_3_STARS_DEGREES) < 0.001) {
        *rise = active_zman_case->night_3_stars - 0.25;
        *set = active_zman_case->night_3_stars;
    } else {
        return -1;
    }

    return 0;
}

static void test_alot_time(void) {
    zman_time_case_t test_case = default_zman_case("test_alot_time", 0, " 500  ");
    assert_zman_time(&test_case);
}

static void test_talis_time(void) {
    zman_time_case_t test_case = default_zman_case("test_talis_time", 1, " 530  ");
    assert_zman_time(&test_case);
}

static void test_netz_time(void) {
    zman_time_case_t test_case = default_zman_case("test_netz_time", 2, " 600  ");
    assert_zman_time(&test_case);
}

static void test_magen_avraham_shema_time(void) {
    zman_time_case_t test_case = default_zman_case("test_magen_avraham_shema_time", 3, " 830  ");
    assert_zman_time(&test_case);
}

static void test_gra_shema_time(void) {
    zman_time_case_t test_case = default_zman_case("test_gra_shema_time", 4, " 900  ");
    assert_zman_time(&test_case);
}

static void test_shacharit_time(void) {
    zman_time_case_t test_case = default_zman_case("test_shacharit_time", 5, "1000  ");
    assert_zman_time(&test_case);
}

static void test_chatzot_time(void) {
    zman_time_case_t test_case = default_zman_case("test_chatzot_time", 6, "1200  ");
    assert_zman_time(&test_case);
}

static void test_mincha_gedolah_time(void) {
    zman_time_case_t test_case = default_zman_case("test_mincha_gedolah_time", 7, "1230  ");
    assert_zman_time(&test_case);
}

static void test_plag_time(void) {
    zman_time_case_t test_case = default_zman_case("test_plag_time", 8, "1645  ");
    assert_zman_time(&test_case);
}

static void test_shkia_time(void) {
    zman_time_case_t test_case = default_zman_case("test_shkia_time", 9, "1800  ");
    assert_zman_time(&test_case);
}

static void test_three_stars_time(void) {
    zman_time_case_t test_case = default_zman_case("test_three_stars_time", 10, "1845  ");
    assert_zman_time(&test_case);
}

static void test_seventy_two_minutes_time(void) {
    zman_time_case_t test_case = default_zman_case("test_seventy_two_minutes_time", 11, "1912  ");
    assert_zman_time(&test_case);
}

static void test_generic_case_with_different_date_location_and_sun_times(void) {
    zman_time_case_t test_case = default_zman_case("test_generic_case_with_different_date_location_and_sun_times", 8, "1745  ");
    test_case.year = 2027;
    test_case.month = 1;
    test_case.day = 5;
    test_case.latitude = 3405;
    test_case.longitude = -11824;
    test_case.dawn = 5.75;
    test_case.night_16_1 = 20.25;
    test_case.talis = 6.25;
    test_case.sunrise = 7.0;
    test_case.sunset = 19.0;
    test_case.night_3_stars = 19.8;

    assert_zman_time(&test_case);
}

static void test_daytime_fast_adds_begin_and_end_fast_labels(void) {
    zman_time_case_t test_case = default_zman_case("test_daytime_fast_adds_begin_and_end_fast_labels", 0, " 500  ");
    test_case.year = 2026;
    test_case.month = 7;
    test_case.day = 2;

    assert_zman_display_at(&test_case, 0, ZMANIM_LABEL_ALOT, false);
    assert_zman_display_at(&test_case, 1, ZMANIM_LABEL_BEGIN_FAST, false);
    assert_zman_display_at(&test_case, 2, " 500  ", true);
    assert_zman_display_at(&test_case, 21, ZMANIM_LABEL_THREE_STARS, false);
    assert_zman_display_at(&test_case, 22, ZMANIM_LABEL_END_FAST, false);
    assert_zman_display_at(&test_case, 23, "1845  ", true);
}

static void test_erev_tisha_bav_adds_begin_fast_between_plag_and_shkia(void) {
    zman_time_case_t test_case = default_zman_case("test_erev_tisha_bav_adds_begin_fast_between_plag_and_shkia", 8, "1645  ");
    test_case.year = 2026;
    test_case.month = 7;
    test_case.day = 22;

    assert_zman_display_at(&test_case, 16, ZMANIM_LABEL_PLAG, false);
    assert_zman_display_at(&test_case, 17, "1645  ", true);
    assert_zman_display_at(&test_case, 18, ZMANIM_LABEL_BEGIN_FAST, false);
    assert_zman_display_at(&test_case, 19, "1742  ", true);
    assert_zman_display_at(&test_case, 20, ZMANIM_LABEL_SHKIA, false);
}

static void test_tisha_bav_adds_end_fast_before_three_stars_time(void) {
    zman_time_case_t test_case = default_zman_case("test_tisha_bav_adds_end_fast_before_three_stars_time", 10, "1845  ");
    test_case.year = 2026;
    test_case.month = 7;
    test_case.day = 23;

    assert_zman_display_at(&test_case, 20, ZMANIM_LABEL_THREE_STARS, false);
    assert_zman_display_at(&test_case, 21, ZMANIM_LABEL_END_FAST, false);
    assert_zman_display_at(&test_case, 22, "1845  ", true);
}

static void test_erev_yom_kippur_adds_begin_fast_and_candle_time(void) {
    zman_time_case_t test_case = default_zman_case("test_erev_yom_kippur_adds_begin_fast_and_candle_time", 8, "1645  ");
    test_case.year = 2026;
    test_case.month = 9;
    test_case.day = 20;

    assert_zman_display_at(&test_case, 16, ZMANIM_LABEL_PLAG, false);
    assert_zman_display_at(&test_case, 17, "1645  ", true);
    assert_zman_display_at(&test_case, 18, ZMANIM_LABEL_BEGIN_FAST, false);
    assert_zman_display_at(&test_case, 19, ZMANIM_LABEL_CANDLE, false);
    assert_zman_display_at(&test_case, 20, "1742  ", true);
    assert_zman_display_at(&test_case, 21, ZMANIM_LABEL_SHKIA, false);
}

static void test_yom_kippur_adds_end_fast_before_three_stars_time(void) {
    zman_time_case_t test_case = default_zman_case("test_yom_kippur_adds_end_fast_before_three_stars_time", 10, "1845  ");
    test_case.year = 2026;
    test_case.month = 9;
    test_case.day = 21;

    assert_zman_display_at(&test_case, 20, ZMANIM_LABEL_THREE_STARS, false);
    assert_zman_display_at(&test_case, 21, ZMANIM_LABEL_END_FAST, false);
    assert_zman_display_at(&test_case, 22, "1845  ", true);
}

static void test_erev_shabbat_adds_candle_time_between_plag_and_shkia(void) {
    zman_time_case_t test_case = default_zman_case("test_erev_shabbat_adds_candle_time_between_plag_and_shkia", 8, "1645  ");
    test_case.year = 2026;
    test_case.month = 7;
    test_case.day = 24;

    assert_zman_display_at(&test_case, 16, ZMANIM_LABEL_PLAG, false);
    assert_zman_display_at(&test_case, 17, "1645  ", true);
    assert_zman_display_at(&test_case, 18, ZMANIM_LABEL_CANDLE, false);
    assert_zman_display_at(&test_case, 19, "1742  ", true);
    assert_zman_display_at(&test_case, 20, ZMANIM_LABEL_SHKIA, false);
}

static void test_erev_holiday_adds_candle_time_between_plag_and_shkia(void) {
    zman_time_case_t test_case = default_zman_case("test_erev_holiday_adds_candle_time_between_plag_and_shkia", 8, "1645  ");
    test_case.year = 2026;
    test_case.month = 4;
    test_case.day = 1;

    assert_zman_display_at(&test_case, 16, ZMANIM_LABEL_PLAG, false);
    assert_zman_display_at(&test_case, 17, "1645  ", true);
    assert_zman_display_at(&test_case, 18, ZMANIM_LABEL_CANDLE, false);
    assert_zman_display_at(&test_case, 19, "1742  ", true);
    assert_zman_display_at(&test_case, 20, ZMANIM_LABEL_SHKIA, false);
}

int main(void) {
    test_alot_time();
    test_talis_time();
    test_netz_time();
    test_magen_avraham_shema_time();
    test_gra_shema_time();
    test_shacharit_time();
    test_chatzot_time();
    test_mincha_gedolah_time();
    test_plag_time();
    test_shkia_time();
    test_three_stars_time();
    test_seventy_two_minutes_time();
    test_generic_case_with_different_date_location_and_sun_times();
    test_daytime_fast_adds_begin_and_end_fast_labels();
    test_erev_tisha_bav_adds_begin_fast_between_plag_and_shkia();
    test_tisha_bav_adds_end_fast_before_three_stars_time();
    test_erev_yom_kippur_adds_begin_fast_and_candle_time();
    test_yom_kippur_adds_end_fast_before_three_stars_time();
    test_erev_shabbat_adds_candle_time_between_plag_and_shkia();
    test_erev_holiday_adds_candle_time_between_plag_and_shkia();

    if (failures) {
        printf("%d Zmanim test failure%s\n", failures, failures == 1 ? "" : "s");
        return 1;
    }

    printf("Zmanim tests passed\n");
    return 0;
}
