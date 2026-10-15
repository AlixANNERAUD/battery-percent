#include "battery_percent.h"

#include <stdio.h>

static int failures = 0;

#define CHECK_EQUAL(actual, expected)                                                                   \
    do                                                                                                  \
    {                                                                                                   \
        long long actual_value = (long long)(actual);                                                   \
        long long expected_value = (long long)(expected);                                               \
        if (actual_value != expected_value)                                                             \
        {                                                                                               \
            fprintf(stderr, "%s:%d: %s == %lld, expected %lld\n", __FILE__, __LINE__, #actual,          \
                    actual_value, expected_value);                                                      \
            failures++;                                                                                 \
        }                                                                                               \
    } while (0)

static void test_percent_clamps_outside_curve(void)
{
    CHECK_EQUAL(battery_percent_from_millivolts(&battery_percent_curve_lipo, 0), 0);
    CHECK_EQUAL(battery_percent_from_millivolts(&battery_percent_curve_lipo, 3306), 0);
    CHECK_EQUAL(battery_percent_from_millivolts(&battery_percent_curve_lipo, 4177), 100);
    CHECK_EQUAL(battery_percent_from_millivolts(&battery_percent_curve_lipo, 5000), 100);
}

static void test_percent_matches_curve_points(void)
{
    CHECK_EQUAL(battery_percent_from_millivolts(&battery_percent_curve_lipo, 3821), 50);
    CHECK_EQUAL(battery_percent_from_millivolts(&battery_percent_curve_li_ion, 3672), 50);
    CHECK_EQUAL(battery_percent_from_millivolts(&battery_percent_curve_lifepo4, 3307), 50);
    CHECK_EQUAL(battery_percent_from_millivolts(&battery_percent_curve_nimh, 1271), 50);
    CHECK_EQUAL(battery_percent_from_millivolts(&battery_percent_curve_alkaline, 1271), 50);
}

static void test_percent_interpolates_between_points(void)
{
    CHECK_EQUAL(battery_percent_from_millivolts(&battery_percent_curve_li_ion, 3717), 55);
    CHECK_EQUAL(battery_percent_from_millivolts(&battery_percent_curve_li_ion, 3379), 5);
}

static void test_percent_is_monotonic_for_builtin_curves(void)
{
    const battery_percent_curve_t *curves[] = {&battery_percent_curve_lipo, &battery_percent_curve_li_ion, &battery_percent_curve_lifepo4,
                                       &battery_percent_curve_nimh, &battery_percent_curve_alkaline};

    for (size_t c = 0; c < sizeof(curves) / sizeof(curves[0]); c++)
    {
        uint8_t previous = 0;
        for (uint32_t mv = 0; mv <= 5000; mv++)
        {
            uint8_t percent = battery_percent_from_millivolts(curves[c], (uint16_t)mv);
            if (percent < previous || percent > 100)
            {
                fprintf(stderr, "curve %zu not monotonic at %u mV\n", c, (unsigned)mv);
                failures++;
                break;
            }
            previous = percent;
        }
    }
}

static void test_percent_handles_invalid_curves(void)
{
    static const battery_percent_point_t single[] = {{3700, 42}};
    static const battery_percent_point_t descending[] = {{4000, 100}, {3000, 0}};
    static const battery_percent_point_t inverted[] = {{3000, 100}, {4000, 0}};
    battery_percent_curve_t empty = {NULL, 0};
    battery_percent_curve_t one = {single, 1};
    battery_percent_curve_t backwards = {descending, 2};
    battery_percent_curve_t falling = {inverted, 2};

    CHECK_EQUAL(battery_percent_from_millivolts(NULL, 3700), 0);
    CHECK_EQUAL(battery_percent_from_millivolts(&empty, 3700), 0);
    CHECK_EQUAL(battery_percent_from_millivolts(&one, 3000), 42);
    CHECK_EQUAL(battery_percent_from_millivolts(&one, 4000), 42);
    CHECK_EQUAL(battery_percent_from_millivolts(&backwards, 3500), 100);
    CHECK_EQUAL(battery_percent_from_millivolts(&falling, 3250), 75);
}

static void test_pack_percent_divides_by_cells(void)
{
    CHECK_EQUAL(battery_percent_from_pack(&battery_percent_curve_lipo, 7642, 2), 50);
    CHECK_EQUAL(battery_percent_from_pack(&battery_percent_curve_lipo, 12600, 3), 100);
    CHECK_EQUAL(battery_percent_from_pack(&battery_percent_curve_lipo, 7642, 0), 0);
    CHECK_EQUAL(battery_percent_from_pack(&battery_percent_curve_lipo, UINT32_MAX, 1), 100);
    CHECK_EQUAL(battery_percent_from_pack(&battery_percent_curve_lipo, UINT32_MAX, 3), 100);
}

static void test_adc_to_millivolts(void)
{
    CHECK_EQUAL(battery_percent_adc_to_millivolts(4095, 12, 3300, 0, 0), 3300);
    CHECK_EQUAL(battery_percent_adc_to_millivolts(0, 12, 3300, 0, 0), 0);
    CHECK_EQUAL(battery_percent_adc_to_millivolts(2048, 12, 3300, 0, 0), 1650);
    CHECK_EQUAL(battery_percent_adc_to_millivolts(2048, 12, 3300, 100000, 100000), 3300);
    CHECK_EQUAL(battery_percent_adc_to_millivolts(4095, 12, 3300, UINT32_MAX, UINT32_MAX), 6600);
    CHECK_EQUAL(battery_percent_adc_to_millivolts(1023, 10, 5000, 0, 0), 5000);
    CHECK_EQUAL(battery_percent_adc_to_millivolts(9999, 12, 3300, 0, 0), 3300);
    CHECK_EQUAL(battery_percent_adc_to_millivolts(100, 0, 3300, 0, 0), 0);
    CHECK_EQUAL(battery_percent_adc_to_millivolts(100, 33, 3300, 0, 0), 0);
    CHECK_EQUAL(battery_percent_adc_to_millivolts(UINT32_MAX, 32, 3300, 0, 0), 3300);
    CHECK_EQUAL(battery_percent_adc_to_millivolts(UINT32_MAX, 32, UINT32_MAX, UINT32_MAX, 1), UINT32_MAX);
}

static void test_filter_smooths_readings(void)
{
    battery_percent_filter_t filter;
    battery_percent_filter_init(&filter, 2);

    CHECK_EQUAL(battery_percent_filter_update(&filter, 4000), 4000);
    CHECK_EQUAL(battery_percent_filter_update(&filter, 4000), 4000);
    CHECK_EQUAL(battery_percent_filter_update(&filter, 3600), 3900);

    for (int i = 0; i < 100; i++)
        battery_percent_filter_update(&filter, 3600);
    CHECK_EQUAL(battery_percent_filter_update(&filter, 3600), 3600);
}

static void test_filter_edge_cases(void)
{
    battery_percent_filter_t filter;

    battery_percent_filter_init(&filter, 0);
    CHECK_EQUAL(battery_percent_filter_update(&filter, 4000), 4000);
    CHECK_EQUAL(battery_percent_filter_update(&filter, 3000), 3000);

    battery_percent_filter_init(&filter, 255);
    CHECK_EQUAL(filter.shift, 16);
    CHECK_EQUAL(battery_percent_filter_update(&filter, UINT16_MAX), UINT16_MAX);
    CHECK_EQUAL(battery_percent_filter_update(&filter, UINT16_MAX), UINT16_MAX);

    CHECK_EQUAL(battery_percent_filter_update(NULL, 1234), 1234);
    battery_percent_filter_init(NULL, 2);
}

int main(void)
{
    test_percent_clamps_outside_curve();
    test_percent_matches_curve_points();
    test_percent_interpolates_between_points();
    test_percent_is_monotonic_for_builtin_curves();
    test_percent_handles_invalid_curves();
    test_pack_percent_divides_by_cells();
    test_adc_to_millivolts();
    test_filter_smooths_readings();
    test_filter_edge_cases();

    if (failures != 0)
    {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    printf("all checks passed\n");
    return 0;
}
