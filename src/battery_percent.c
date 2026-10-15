#include "battery_percent.h"

#define BATTERY_PERCENT_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define BATTERY_PERCENT_FILTER_MAX_SHIFT 16u

static const battery_percent_point_t lipo_points[] = {
    {3306, 0}, {3675, 5}, {3687, 10}, {3710, 15}, {3741, 20}, {3762, 25}, {3775, 30},
    {3784, 35}, {3793, 40}, {3806, 45}, {3821, 50}, {3841, 55}, {3884, 60}, {3918, 65},
    {3945, 70}, {3975, 75}, {4008, 80}, {4045, 85}, {4086, 90}, {4129, 95}, {4177, 100},
};

static const battery_percent_point_t li_ion_points[] = {
    {3281, 0}, {3477, 10}, {3569, 20}, {3606, 30}, {3633, 40}, {3672, 50}, {3762, 60},
    {3849, 70}, {3949, 80}, {4060, 90}, {4180, 100},
};

static const battery_percent_point_t lifepo4_points[] = {
    {2500, 0}, {3098, 5}, {3209, 10}, {3226, 15}, {3251, 20}, {3269, 25}, {3283, 30},
    {3296, 35}, {3303, 40}, {3305, 45}, {3307, 50}, {3308, 55}, {3310, 60}, {3313, 65},
    {3325, 70}, {3342, 75}, {3346, 80}, {3348, 85}, {3352, 90}, {3366, 95}, {3500, 100},
};

static const battery_percent_point_t nimh_points[] = {
    {1000, 0}, {1154, 5}, {1191, 10}, {1215, 15}, {1232, 20}, {1244, 25}, {1253, 30},
    {1260, 35}, {1265, 40}, {1268, 45}, {1271, 50}, {1273, 60}, {1275, 75}, {1279, 80},
    {1290, 85}, {1309, 90}, {1336, 95}, {1440, 100},
};

static const battery_percent_point_t alkaline_points[] = {
    {900, 0}, {989, 5}, {1051, 10}, {1100, 15}, {1147, 20}, {1184, 25}, {1208, 30},
    {1226, 35}, {1241, 40}, {1256, 45}, {1271, 50}, {1286, 55}, {1303, 60}, {1321, 65},
    {1340, 70}, {1360, 75}, {1382, 80}, {1406, 85}, {1435, 90}, {1467, 95}, {1500, 100},
};

const battery_percent_curve_t battery_percent_curve_lipo = {lipo_points, BATTERY_PERCENT_COUNT(lipo_points)};
const battery_percent_curve_t battery_percent_curve_li_ion = {li_ion_points, BATTERY_PERCENT_COUNT(li_ion_points)};
const battery_percent_curve_t battery_percent_curve_lifepo4 = {lifepo4_points, BATTERY_PERCENT_COUNT(lifepo4_points)};
const battery_percent_curve_t battery_percent_curve_nimh = {nimh_points, BATTERY_PERCENT_COUNT(nimh_points)};
const battery_percent_curve_t battery_percent_curve_alkaline = {alkaline_points, BATTERY_PERCENT_COUNT(alkaline_points)};

static int32_t divide_rounded(int32_t numerator, int32_t denominator)
{
    if (numerator >= 0)
        return (numerator + denominator / 2) / denominator;
    return (numerator - denominator / 2) / denominator;
}

uint8_t battery_percent_from_millivolts(const battery_percent_curve_t *curve, uint16_t millivolts)
{
    if (curve == NULL || curve->points == NULL || curve->count == 0)
        return 0;

    const battery_percent_point_t *points = curve->points;
    size_t last = curve->count - 1;

    if (millivolts <= points[0].millivolts)
        return points[0].percent;
    if (millivolts >= points[last].millivolts)
        return points[last].percent;

    size_t upper = 1;
    while (upper < last && points[upper].millivolts < millivolts)
        upper++;

    const battery_percent_point_t *low = &points[upper - 1];
    const battery_percent_point_t *high = &points[upper];

    if (high->millivolts <= low->millivolts)
        return high->percent;

    int32_t span_millivolts = (int32_t)high->millivolts - (int32_t)low->millivolts;
    int32_t span_percent = (int32_t)high->percent - (int32_t)low->percent;
    int32_t offset = (int32_t)millivolts - (int32_t)low->millivolts;
    int32_t percent = (int32_t)low->percent + divide_rounded(span_percent * offset, span_millivolts);

    if (percent < 0)
        return 0;
    if (percent > 100)
        return 100;
    return (uint8_t)percent;
}

uint8_t battery_percent_from_pack(const battery_percent_curve_t *curve, uint32_t millivolts, uint8_t cells)
{
    if (cells == 0)
        return 0;

    uint64_t cell_millivolts = ((uint64_t)millivolts + cells / 2u) / cells;
    if (cell_millivolts > UINT16_MAX)
        cell_millivolts = UINT16_MAX;

    return battery_percent_from_millivolts(curve, (uint16_t)cell_millivolts);
}

uint32_t battery_percent_adc_to_millivolts(uint32_t raw, uint8_t resolution_bits, uint32_t reference_millivolts, uint32_t resistor_1, uint32_t resistor_2)
{
    if (resolution_bits == 0 || resolution_bits > 32)
        return 0;

    uint64_t full_scale = (UINT64_C(1) << resolution_bits) - 1u;
    if ((uint64_t)raw > full_scale)
        raw = (uint32_t)full_scale;

    uint64_t millivolts = ((uint64_t)raw * reference_millivolts + full_scale / 2u) / full_scale;

    if (resistor_2 != 0)
    {
        millivolts += (millivolts * resistor_1 + resistor_2 / 2u) / resistor_2;
    }

    if (millivolts > UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)millivolts;
}

void battery_percent_filter_init(battery_percent_filter_t *filter, uint8_t shift)
{
    if (filter == NULL)
        return;

    filter->state = 0;
    filter->shift = shift > BATTERY_PERCENT_FILTER_MAX_SHIFT ? BATTERY_PERCENT_FILTER_MAX_SHIFT : shift;
    filter->primed = 0;
}

uint16_t battery_percent_filter_update(battery_percent_filter_t *filter, uint16_t millivolts)
{
    if (filter == NULL)
        return millivolts;

    uint8_t shift = filter->shift > BATTERY_PERCENT_FILTER_MAX_SHIFT ? BATTERY_PERCENT_FILTER_MAX_SHIFT : filter->shift;

    if (!filter->primed)
    {
        filter->state = (uint32_t)millivolts << shift;
        filter->primed = 1;
        return millivolts;
    }

    filter->state = filter->state - (filter->state >> shift) + millivolts;

    return (uint16_t)(filter->state >> shift);
}
