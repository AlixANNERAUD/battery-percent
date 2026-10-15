/**
 * @file battery_percent.h
 * @brief Hardware-agnostic battery state-of-charge estimation from cell voltage.
 *
 * The caller measures the battery voltage in millivolts, and the library converts it into a charge percentage using a
 * discharge curve: a table of voltage and percentage points with linear interpolation between them. The library is
 * integer-only C99, does not allocate and has no platform dependency.
 */

#ifndef BATTERY_PERCENT_H
#define BATTERY_PERCENT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief One point of a discharge curve.
 */
typedef struct
{
    /** Cell voltage in millivolts. */
    uint16_t millivolts;
    /** Charge percentage at that voltage, from 0 to 100. */
    uint8_t percent;
} battery_percent_point_t;

/**
 * @brief A discharge curve: points sorted by strictly increasing voltage.
 *
 * Percentages should increase with voltage. The curve does not own its points, which must outlive it. Build custom
 * curves from a static array:
 *
 * @code
 * static const battery_percent_point_t points[] = {{3000, 0}, {3600, 20}, {3900, 80}, {4100, 100}};
 * static const battery_percent_curve_t my_cell = {points, sizeof(points) / sizeof(points[0])};
 * @endcode
 */
typedef struct
{
    /** Points sorted by increasing @ref battery_percent_point_t::millivolts. */
    const battery_percent_point_t *points;
    /** Number of points. */
    size_t count;
} battery_percent_curve_t;

/**
 * @brief Integer exponential moving average filter for voltage readings.
 *
 * Initialise it with battery_percent_filter_init() and feed readings with battery_percent_filter_update(). The fields
 * are internal state and should not be modified directly.
 */
typedef struct
{
    /** Accumulated value, scaled by 2^shift. */
    uint32_t state;
    /** Smoothing strength: each new reading has a weight of 1 / 2^shift. */
    uint8_t shift;
    /** Non-zero once the first reading has been received. */
    uint8_t primed;
} battery_percent_filter_t;

/**
 * @brief Lithium polymer (LiPo) cell, 3306 to 4177 mV.
 *
 * Open-circuit voltage after a 60 minute rest of a typical Li-ion polymer cell, from Maxim/ADI Application Note 4189.
 */
extern const battery_percent_curve_t battery_percent_curve_lipo;

/**
 * @brief Cylindrical Li-ion NMC cell such as an 18650, 3281 to 4180 mV.
 *
 * Open-circuit voltage after a 2 hour rest of a Samsung INR18650-20R, from CALCE incremental OCV data. 0% is the
 * 2.5 V cutoff under a 0.5C load.
 */
extern const battery_percent_curve_t battery_percent_curve_li_ion;

/**
 * @brief Lithium iron phosphate (LiFePO4) cell, 2500 to 3500 mV.
 *
 * Average of C/20 charge and discharge curves of A123 cells, from CALCE low-current OCV data. The curve is nearly flat
 * between 40% and 90%, where voltage can only resolve the charge coarsely.
 */
extern const battery_percent_curve_t battery_percent_curve_lifepo4;

/**
 * @brief Nickel-metal hydride (NiMH) AA cell, 1000 to 1440 mV.
 *
 * Voltage under a 0.1C load of an Energizer NH15-2300, from its datasheet.
 */
extern const battery_percent_curve_t battery_percent_curve_nimh;

/**
 * @brief Alkaline AA cell, 900 to 1500 mV.
 *
 * Voltage under a 50 mA intermittent load of an Energizer E91, from its datasheet, with 0% at 0.9 V.
 */
extern const battery_percent_curve_t battery_percent_curve_alkaline;

/**
 * @brief Charge percentage of a single cell.
 *
 * Interpolates linearly between the two curve points surrounding @p millivolts and rounds to the nearest percent.
 * Voltages below the first point return its percentage, and voltages above the last point return its percentage.
 *
 * @param curve Discharge curve, for example &battery_percent_curve_lipo.
 * @param millivolts Cell voltage in millivolts.
 * @return Charge percentage from 0 to 100, or 0 when @p curve is NULL or empty.
 */
uint8_t battery_percent_from_millivolts(const battery_percent_curve_t *curve, uint16_t millivolts);

/**
 * @brief Charge percentage of a pack of identical cells in series.
 *
 * Divides the pack voltage by the number of cells, rounding to the nearest millivolt, then calls
 * battery_percent_from_millivolts().
 *
 * @param curve Discharge curve of one cell.
 * @param millivolts Pack voltage in millivolts.
 * @param cells Number of cells in series, for example 3 for a 3S pack.
 * @return Charge percentage from 0 to 100, or 0 when @p cells is 0.
 */
uint8_t battery_percent_from_pack(const battery_percent_curve_t *curve, uint32_t millivolts, uint8_t cells);

/**
 * @brief Converts a raw ADC reading into the battery voltage.
 *
 * Scales @p raw so that the full-scale reading (2^resolution_bits - 1) equals @p reference_millivolts, then undoes a
 * voltage divider made of @p resistor_1 between the battery and the ADC pin and @p resistor_2 between the pin and
 * ground. Readings above full scale are clamped, and results are rounded to the nearest millivolt.
 *
 * @code
 * uint32_t millivolts = battery_percent_adc_to_millivolts(raw, 12, 3300, 100000, 100000);
 * @endcode
 *
 * @param raw Raw ADC reading.
 * @param resolution_bits ADC resolution, from 1 to 32 bits.
 * @param reference_millivolts ADC reference voltage in millivolts.
 * @param resistor_1 Resistor between the battery and the ADC pin, in any unit.
 * @param resistor_2 Resistor between the ADC pin and ground, in the same unit, or 0 without a divider.
 * @return Battery voltage in millivolts, saturated to UINT32_MAX, or 0 when @p resolution_bits is out of range.
 */
uint32_t battery_percent_adc_to_millivolts(uint32_t raw, uint8_t resolution_bits, uint32_t reference_millivolts, uint32_t resistor_1, uint32_t resistor_2);

/**
 * @brief Initialises a moving average filter.
 *
 * Each reading has a weight of 1 / 2^shift: 0 disables smoothing, and higher values smooth more but react more
 * slowly. Does nothing when @p filter is NULL.
 *
 * @param filter Filter to initialise.
 * @param shift Smoothing strength, clamped to 16.
 */
void battery_percent_filter_init(battery_percent_filter_t *filter, uint8_t shift);

/**
 * @brief Feeds a reading to a moving average filter.
 *
 * The first reading after battery_percent_filter_init() is returned unchanged and seeds the filter.
 *
 * @param filter Filter initialised with battery_percent_filter_init().
 * @param millivolts New voltage reading in millivolts.
 * @return Smoothed voltage in millivolts, or @p millivolts unchanged when @p filter is NULL.
 */
uint16_t battery_percent_filter_update(battery_percent_filter_t *filter, uint16_t millivolts);

#ifdef __cplusplus
}
#endif

#endif
