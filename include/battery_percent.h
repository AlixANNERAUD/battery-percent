#ifndef BATTERY_PERCENT_H
#define BATTERY_PERCENT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t millivolts;
    uint8_t percent;
} battery_percent_point_t;

typedef struct
{
    const battery_percent_point_t *points;
    size_t count;
} battery_percent_curve_t;

typedef struct
{
    uint32_t state;
    uint8_t shift;
    uint8_t primed;
} battery_percent_filter_t;

extern const battery_percent_curve_t battery_percent_curve_lipo;
extern const battery_percent_curve_t battery_percent_curve_li_ion;
extern const battery_percent_curve_t battery_percent_curve_lifepo4;
extern const battery_percent_curve_t battery_percent_curve_nimh;
extern const battery_percent_curve_t battery_percent_curve_alkaline;

uint8_t battery_percent_from_millivolts(const battery_percent_curve_t *curve, uint16_t millivolts);

uint8_t battery_percent_from_pack(const battery_percent_curve_t *curve, uint32_t millivolts, uint8_t cells);

uint32_t battery_percent_adc_to_millivolts(uint32_t raw, uint8_t resolution_bits, uint32_t reference_millivolts, uint32_t resistor_1, uint32_t resistor_2);

void battery_percent_filter_init(battery_percent_filter_t *filter, uint8_t shift);

uint16_t battery_percent_filter_update(battery_percent_filter_t *filter, uint16_t millivolts);

#ifdef __cplusplus
}
#endif

#endif
