/*
 * Host-side tests for the Hebrew date logic in hebrew_date_face.c.
 *
 * Example build from the repository root:
 *   gcc -std=c99 -Wall -Wextra \
 *     -Iwatch-faces/test/include \
 *     -Iwatch-faces/complication \
 *     watch-faces/test/hebrew_date_test.c \
 *     -o /tmp/hebrew_date_test
 *
 * Example run:
 *   /tmp/hebrew_date_test
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../complication/hebrew_date_face.c"

static int failures = 0;
static watch_date_time_t fake_now;
static movement_location_t fake_location;
static bool fake_pm_indicator;
static uint8_t location_begin_calls;
static char displayed_top_left[8];
static char displayed_top_right[8];
static char displayed_bottom[8];

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), #actual, __LINE__)
#define ASSERT_EQ_STR(expected, actual) assert_eq_str((expected), (actual), #actual, __LINE__)
#define ASSERT_TRUE(actual) assert_eq_bool(true, (actual), #actual, __LINE__)
#define ASSERT_FALSE(actual) assert_eq_bool(false, (actual), #actual, __LINE__)

static void assert_eq_int(int32_t expected, int32_t actual, const char *expr, int line) {
    if (expected != actual) {
        printf("line %d: expected %s to be %ld, got %ld\n",
               line, expr, (long)expected, (long)actual);
        failures++;
    }
}

static void assert_eq_bool(bool expected, bool actual, const char *expr, int line) {
    if (expected != actual) {
        printf("line %d: expected %s to be %s, got %s\n",
               line, expr, expected ? "true" : "false", actual ? "true" : "false");
        failures++;
    }
}

static void assert_eq_str(const char *expected, const char *actual, const char *expr, int line) {
    if (strcmp(expected, actual) != 0) {
        printf("line %d: expected %s to be \"%s\", got \"%s\"\n",
               line, expr, expected, actual);
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

void watch_display_text(watch_position_t location, const char *string) {
    char *target = NULL;

    if (location == WATCH_POSITION_TOP_LEFT) target = displayed_top_left;
    else if (location == WATCH_POSITION_TOP_RIGHT) target = displayed_top_right;
    else if (location == WATCH_POSITION_BOTTOM) target = displayed_bottom;

    if (target) {
        snprintf(target, 8, "%s", string);
    }
}

void watch_set_indicator(watch_indicator_t indicator) {
    if (indicator == WATCH_INDICATOR_PM) fake_pm_indicator = true;
}

void watch_clear_indicator(watch_indicator_t indicator) {
    if (indicator == WATCH_INDICATOR_PM) fake_pm_indicator = false;
}

bool movement_default_loop_handler(movement_event_t event) {
    (void)event;
    return true;
}

bool filesystem_read_file(char *filename, char *buf, int32_t length) {
    (void)filename;
    memcpy(buf, &fake_location.reg, length);
    return fake_location.reg != 0;
}

bool filesystem_write_file(char *filename, char *buf, int32_t length) {
    (void)filename;
    (void)buf;
    (void)length;
    return true;
}

movement_location_t location_settings_load_location(void) {
    return fake_location;
}

void location_settings_load_working_location(location_settings_state_t *state) {
    (void)state;
}

void location_settings_persist_if_changed(location_settings_state_t *state) {
    (void)state;
}

bool location_settings_is_active(location_settings_state_t *state) {
    return state->page != 0;
}

void location_settings_begin(location_settings_state_t *state, movement_event_t event) {
    (void)event;
    state->page = 1;
    state->active_digit = 0;
    location_begin_calls++;
}

bool location_settings_handle_event(location_settings_state_t *state, movement_event_t event, bool *finished) {
    *finished = false;

    if (!location_settings_is_active(state)) return false;

    switch (event.event_type) {
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_ALARM_BUTTON_UP:
            return true;
        case EVENT_ALARM_LONG_PRESS:
        case EVENT_TIMEOUT:
            state->page = 0;
            *finished = true;
            return true;
        default:
            return false;
    }
}

void movement_request_tick_frequency(uint8_t freq) {
    (void)freq;
}

watch_lcd_type_t watch_get_lcd_type(void) {
    return WATCH_LCD_TYPE_CLASSIC;
}

void watch_clear_display(void) {
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
    (void)upper_limb;
    if (altit < -1.0) {
        *rise = 5.0;
        *set = 19.0;
    } else {
        *rise = 6.0;
        *set = 18.0;
    }
    return 0;
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

    fake_pm_indicator = false;
    location_begin_calls = 0;
    memset(displayed_top_left, 0, sizeof(displayed_top_left));
    memset(displayed_top_right, 0, sizeof(displayed_top_right));
    memset(displayed_bottom, 0, sizeof(displayed_bottom));
}

static uint8_t hebrew_month_count(uint16_t year) {
    return hebrew_date_is_leap_year(year) ? 13 : 12;
}

static uint16_t hebrew_year_length_from_months(uint16_t year) {
    uint16_t length = 0;
    for (uint8_t month = 0; month < hebrew_month_count(year); month++) {
        length += hebrew_date_month_length(year, month);
    }
    return length;
}

static void test_hebrew_date_is_leap_year(void) {
    ASSERT_TRUE(hebrew_date_is_leap_year(5784));
    ASSERT_FALSE(hebrew_date_is_leap_year(5785));
    ASSERT_FALSE(hebrew_date_is_leap_year(5786));
    ASSERT_TRUE(hebrew_date_is_leap_year(5787));

    for (uint16_t year = 5700; year < 5800; year++) {
        ASSERT_EQ_INT(hebrew_date_is_leap_year(year),
                      hebrew_date_is_leap_year(year + 19));
    }
}

static void test_leap_years_vs_non_leap_years(void) {
    ASSERT_TRUE(hebrew_date_is_leap_year(5784));
    ASSERT_EQ_INT(13, hebrew_month_count(5784));
    ASSERT_EQ_INT(30, hebrew_date_month_length(5784, 5)); // Adar I
    ASSERT_EQ_INT(29, hebrew_date_month_length(5784, 6)); // Adar II

    ASSERT_FALSE(hebrew_date_is_leap_year(5785));
    ASSERT_EQ_INT(12, hebrew_month_count(5785));
    ASSERT_EQ_INT(29, hebrew_date_month_length(5785, 5)); // Adar
    ASSERT_EQ_INT(30, hebrew_date_month_length(5785, 6)); // Nisan
}

static void test_hebrew_date_month_length(void) {
    const uint8_t lengths_5784[] = {
        30, 29, 29, 29, 30, 30, 29, 30, 29, 30, 29, 30, 29
    };
    const uint8_t lengths_5785[] = {
        30, 30, 30, 29, 30, 29, 30, 29, 30, 29, 30, 29
    };

    for (uint8_t month = 0; month < sizeof(lengths_5784); month++) {
        ASSERT_EQ_INT(lengths_5784[month], hebrew_date_month_length(5784, month));
    }

    for (uint8_t month = 0; month < sizeof(lengths_5785); month++) {
        ASSERT_EQ_INT(lengths_5785[month], hebrew_date_month_length(5785, month));
    }

    ASSERT_EQ_INT(383, hebrew_year_length_from_months(5784));
    ASSERT_EQ_INT(355, hebrew_year_length_from_months(5785));
}

static void test_cheshvan_kislev_short_regular_complete_years(void) {
    ASSERT_EQ_INT(383, hebrew_year_length_from_months(5784)); // short leap year
    ASSERT_EQ_INT(29, hebrew_date_month_length(5784, 1));     // Cheshvan
    ASSERT_EQ_INT(29, hebrew_date_month_length(5784, 2));     // Kislev

    ASSERT_EQ_INT(354, hebrew_year_length_from_months(5786)); // regular common year
    ASSERT_EQ_INT(29, hebrew_date_month_length(5786, 1));     // Cheshvan
    ASSERT_EQ_INT(30, hebrew_date_month_length(5786, 2));     // Kislev

    ASSERT_EQ_INT(355, hebrew_year_length_from_months(5785)); // complete common year
    ASSERT_EQ_INT(30, hebrew_date_month_length(5785, 1));     // Cheshvan
    ASSERT_EQ_INT(30, hebrew_date_month_length(5785, 2));     // Kislev
}

static void test_hebrew_date_fixed_from_gregorian(void) {
    ASSERT_EQ_INT(1, hebrew_date_fixed_from_gregorian(1, 1, 1));
    ASSERT_EQ_INT(730120, hebrew_date_fixed_from_gregorian(2000, 1, 1));
    ASSERT_EQ_INT(738945, hebrew_date_fixed_from_gregorian(2024, 2, 29));
}

static void assert_hebrew_date(uint16_t gy, uint8_t gm, uint8_t gd,
                               uint16_t hy, uint8_t hm, uint8_t hd, uint8_t weekday) {
    hebrew_date_t actual = hebrew_date_from_gregorian(gy, gm, gd);

    ASSERT_EQ_INT(hy, actual.year);
    ASSERT_EQ_INT(hm, actual.month);
    ASSERT_EQ_INT(hd, actual.day);
    ASSERT_EQ_INT(weekday, actual.weekday);
}

static void test_hebrew_date_from_gregorian(void) {
    assert_hebrew_date(2024, 2, 10, 5784, 5, 1, 7);   // Rosh Chodesh Adar I
    assert_hebrew_date(2024, 3, 11, 5784, 6, 1, 2);   // Rosh Chodesh Adar II
    assert_hebrew_date(2024, 3, 24, 5784, 6, 14, 1);  // Purim
    assert_hebrew_date(2024, 4, 23, 5784, 7, 15, 3);  // Pesach
    assert_hebrew_date(2025, 4, 13, 5785, 6, 15, 1);  // Pesach, non-leap year
}

static void test_rosh_hashanah_boundaries(void) {
    assert_hebrew_date(2023, 9, 15, 5783, 11, 29, 6);  // Last day of 5783
    assert_hebrew_date(2023, 9, 16, 5784, 0, 1, 7);    // Rosh Hashanah 5784
    assert_hebrew_date(2023, 9, 17, 5784, 0, 2, 1);    // Second day of 5784

    assert_hebrew_date(2024, 10, 2, 5784, 12, 29, 4);  // Last day of 5784
    assert_hebrew_date(2024, 10, 3, 5785, 0, 1, 5);    // Rosh Hashanah 5785
    assert_hebrew_date(2024, 10, 4, 5785, 0, 2, 6);    // Second day of 5785
}

static void test_adar_adar_i_adar_ii_behavior(void) {
    assert_hebrew_date(2024, 2, 9, 5784, 4, 30, 6);    // Last day of Shevat
    assert_hebrew_date(2024, 2, 10, 5784, 5, 1, 7);    // Adar I in leap year
    assert_hebrew_date(2024, 3, 10, 5784, 5, 30, 1);   // Last day of Adar I
    assert_hebrew_date(2024, 3, 11, 5784, 6, 1, 2);    // Adar II in leap year
    assert_hebrew_date(2024, 4, 8, 5784, 6, 29, 2);    // Last day of Adar II
    assert_hebrew_date(2024, 4, 9, 5784, 7, 1, 3);     // Nisan after Adar II

    assert_hebrew_date(2025, 3, 1, 5785, 5, 1, 7);     // Adar in non-leap year
    assert_hebrew_date(2025, 3, 29, 5785, 5, 29, 7);   // Last day of Adar
    assert_hebrew_date(2025, 3, 30, 5785, 6, 1, 1);    // Nisan after Adar
}

static void test_gregorian_leap_day(void) {
    ASSERT_EQ_INT(738945, hebrew_date_fixed_from_gregorian(2024, 2, 29));
    assert_hebrew_date(2024, 2, 28, 5784, 5, 19, 4);
    assert_hebrew_date(2024, 2, 29, 5784, 5, 20, 5);
    assert_hebrew_date(2024, 3, 1, 5784, 5, 21, 6);
}

static void test_face_says_no_location_without_shared_lat_lon(void) {
    hebrew_date_state_t state = {0};

    reset_face_test_state(2024, 10, 2, 12, 0, false);
    hebrew_date_update_display(&state);

    ASSERT_EQ_STR("HE", displayed_top_left);
    ASSERT_EQ_STR("  ", displayed_top_right);
    ASSERT_EQ_STR("No LOC", displayed_bottom);
    ASSERT_FALSE(fake_pm_indicator);
}

static void test_face_uses_civil_date_before_sunset(void) {
    hebrew_date_state_t state = {0};

    reset_face_test_state(2024, 10, 2, 17, 59, true);
    hebrew_date_update_display(&state);

    ASSERT_EQ_STR("29", displayed_top_right);
    ASSERT_FALSE(fake_pm_indicator);
}

static void test_face_advances_hebrew_date_after_sunset(void) {
    hebrew_date_state_t state = {0};

    reset_face_test_state(2024, 10, 2, 18, 0, true);
    hebrew_date_update_display(&state);

    ASSERT_EQ_STR(" 1", displayed_top_right);
    ASSERT_TRUE(fake_pm_indicator);
}

static void test_face_pm_indicator_stays_on_until_alot(void) {
    hebrew_date_state_t state = {0};

    reset_face_test_state(2024, 10, 3, 4, 59, true);
    hebrew_date_update_display(&state);

    ASSERT_EQ_STR(" 1", displayed_top_right);
    ASSERT_TRUE(fake_pm_indicator);

    reset_face_test_state(2024, 10, 3, 5, 0, true);
    hebrew_date_update_display(&state);

    ASSERT_EQ_STR(" 1", displayed_top_right);
    ASSERT_FALSE(fake_pm_indicator);
}

static void test_alarm_button_toggles_hebrew_year(void) {
    hebrew_date_state_t state = {0};

    reset_face_test_state(2024, 10, 2, 12, 0, true);
    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ACTIVATE }, &state);

    ASSERT_EQ_STR("Elul  ", displayed_bottom);

    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ALARM_BUTTON_UP }, &state);
    ASSERT_TRUE(state.show_year);
    ASSERT_EQ_STR("  5784", displayed_bottom);

    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ALARM_BUTTON_UP }, &state);
    ASSERT_FALSE(state.show_year);
    ASSERT_EQ_STR("Elul  ", displayed_bottom);
}

static void test_alarm_long_press_still_opens_location_settings(void) {
    hebrew_date_state_t state = {0};

    reset_face_test_state(2024, 10, 2, 12, 0, true);
    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ACTIVATE }, &state);
    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ALARM_LONG_PRESS }, &state);
    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ALARM_LONG_UP }, &state);

    ASSERT_EQ_INT(1, location_begin_calls);
    ASSERT_FALSE(state.show_year);
}

static void test_opening_alarm_hold_does_not_close_location_settings(void) {
    hebrew_date_state_t state = {0};

    reset_face_test_state(2024, 10, 2, 12, 0, false);
    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ACTIVATE }, &state);

    ASSERT_EQ_STR("No LOC", displayed_bottom);

    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ALARM_LONG_PRESS }, &state);
    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ALARM_LONG_PRESS }, &state);
    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ALARM_REALLY_LONG_PRESS }, &state);

    ASSERT_EQ_INT(1, location_begin_calls);
    ASSERT_TRUE(location_settings_is_active(&state.location_settings));
    ASSERT_EQ_STR("No LOC", displayed_bottom);

    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ALARM_LONG_UP }, &state);
    hebrew_date_face_loop((movement_event_t){ .event_type = EVENT_ALARM_LONG_PRESS }, &state);

    ASSERT_FALSE(location_settings_is_active(&state.location_settings));
}

int main(void) {
    test_hebrew_date_is_leap_year();
    test_leap_years_vs_non_leap_years();
    test_hebrew_date_month_length();
    test_cheshvan_kislev_short_regular_complete_years();
    test_hebrew_date_fixed_from_gregorian();
    test_hebrew_date_from_gregorian();
    test_rosh_hashanah_boundaries();
    test_adar_adar_i_adar_ii_behavior();
    test_gregorian_leap_day();
    test_face_says_no_location_without_shared_lat_lon();
    test_face_uses_civil_date_before_sunset();
    test_face_advances_hebrew_date_after_sunset();
    test_face_pm_indicator_stays_on_until_alot();
    test_alarm_button_toggles_hebrew_year();
    test_alarm_long_press_still_opens_location_settings();
    test_opening_alarm_hold_does_not_close_location_settings();

    if (failures) {
        printf("%d Hebrew date test failure%s\n", failures, failures == 1 ? "" : "s");
        return 1;
    }

    printf("Hebrew date tests passed\n");
    return 0;
}
