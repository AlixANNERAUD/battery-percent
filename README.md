# battery-percent

[![CI](https://github.com/AlixANNERAUD/battery-percent/actions/workflows/ci.yml/badge.svg)](https://github.com/AlixANNERAUD/battery-percent/actions/workflows/ci.yml)
[![Rust](https://github.com/AlixANNERAUD/battery-percent/actions/workflows/rust.yml/badge.svg)](https://github.com/AlixANNERAUD/battery-percent/actions/workflows/rust.yml)
[![codecov](https://codecov.io/gh/AlixANNERAUD/battery-percent/graph/badge.svg)](https://codecov.io/gh/AlixANNERAUD/battery-percent)
[![crates.io](https://img.shields.io/crates/v/battery-percent.svg)](https://crates.io/crates/battery-percent)
[![docs.rs](https://img.shields.io/docsrs/battery-percent)](https://docs.rs/battery-percent)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/alix-anneraud/library/battery-percent.svg)](https://registry.platformio.org/libraries/alix-anneraud/battery-percent)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/AlixANNERAUD/battery-percent/blob/main/LICENSE)

**Turn a battery voltage into a realistic charge percentage on any microcontroller, in one function call.**
`battery-percent` is a tiny C99 library with `no_std` Rust bindings. It converts a LiPo, Li-ion 18650, LiFePO4, NiMH
or alkaline cell voltage into a state-of-charge percentage using real discharge curves instead of a misleading
straight line. Read the voltage however your board allows (ESP32, STM32, RP2040, nRF52, AVR, Linux…) and let the
library do the rest.

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/AlixANNERAUD/battery-percent/main/docs/curves-dark.svg">
    <img src="https://raw.githubusercontent.com/AlixANNERAUD/battery-percent/main/docs/curves-light.svg" alt="Built-in battery discharge curves: voltage against state of charge for LiPo, Li-ion 18650, LiFePO4, NiMH and alkaline cells" width="960">
  </picture>
</p>

## Why battery-percent?

- **Real curves, not `map(voltage, 3000, 4200, 0, 100)`.** A lithium cell spends most of its life between 3.6 and
  3.9 V, so a linear mapping shows a half-full battery that is about to die. Every built-in curve comes from
  datasheets or lab measurements, and the library interpolates between its points.
- **Hardware-agnostic.** No `analogRead`, no HAL, no platform header: pass millivolts, get a percentage. The same code
  runs on every MCU, on Linux and in unit tests on your PC.
- **Five chemistries built in.** LiPo, Li-ion 18650 (NMC), LiFePO4, NiMH and alkaline, plus your own curve once you
  have measured your cells.
- **Packs and voltage dividers.** Series packs (2S, 3S, 4S…) and a raw ADC → millivolts helper that accounts for your
  resistor divider.
- **Stable readings.** An integer moving average filter smooths ADC noise without floating point.
- **Tiny.** Integer-only C99, no allocation, under 1 KB of flash on a Cortex-M4 with every curve included. Tested
  under address and undefined-behaviour sanitizers.
- **C, C++, Rust and beyond.** A plain C ABI callable from any language with an FFI, and a `no_std` Rust crate on
  [crates.io](https://crates.io/crates/battery-percent).
- **Ready for your toolchain.** PlatformIO (any platform and framework, including Arduino), CMake (ESP-IDF, Zephyr,
  Pico SDK, desktop) and Cargo.

## Quick start

### Arduino / ESP32

```cpp
#include <Arduino.h>
#include <battery_percent.h>

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    uint16_t millivolts = analogReadMilliVolts(34) * 2;
    uint8_t percent = battery_percent_from_millivolts(&battery_percent_curve_lipo, millivolts);
    Serial.printf("Battery: %u mV, %u%%\n", millivolts, percent);
    delay(1000);
}
```

The battery is wired to GPIO 34 through a 100 kΩ / 100 kΩ voltage divider, hence the `* 2`.

### C / C++ on any MCU

```c
#include "battery_percent.h"

uint32_t raw = read_adc();
uint32_t millivolts = battery_percent_adc_to_millivolts(raw, 12, 3300, 100000, 100000);

uint8_t percent = battery_percent_from_millivolts(&battery_percent_curve_li_ion, (uint16_t)millivolts);
uint8_t pack = battery_percent_from_pack(&battery_percent_curve_lipo, 11100, 3);
```

`battery_percent_adc_to_millivolts(raw, resolution_bits, reference_mv, r1, r2)` converts a raw reading, here from a
12-bit ADC with a 3.3 V reference behind a divider made of `r1` (battery to pin) and `r2` (pin to ground). Pass
`r2 = 0` without a divider. Skip it if your platform already returns calibrated millivolts.

### Rust

```toml
[dependencies]
battery-percent = "2"
```

```rust
use battery_percent::{adc_to_millivolts, Curve, Filter, Point};

let millivolts = adc_to_millivolts(2383, 12, 3300, 100_000, 100_000) as u16;

let mut filter = Filter::new(3);
let smoothed = filter.update(millivolts);

let percent = Curve::lipo().percent(smoothed);
assert!(percent <= 100);

const MY_CELL: [Point; 3] = [Point::new(3000, 0), Point::new(3700, 50), Point::new(4200, 100)];
let custom = Curve::new(&MY_CELL);
assert_eq!(custom.percent(3700), 50);
assert_eq!(custom.pack_percent(7400, 2), 50);
```

The crate is `no_std` and compiles the C core with [`cc`](https://crates.io/crates/cc), so cross-compiling needs a
C compiler for the target (for example `arm-none-eabi-gcc`). Raw bindings live in `battery_percent::ffi`.

## Installing

### PlatformIO

```ini
lib_deps = alix-anneraud/battery-percent
```

Works with every platform and framework: Arduino, ESP-IDF, STM32Cube, Zephyr, native…

### CMake (ESP-IDF, Zephyr, Pico SDK, desktop)

```cmake
add_subdirectory(battery-percent)
target_link_libraries(app PRIVATE battery_percent::battery_percent)
```

### Anything else

Copy `include/battery_percent.h` and `src/battery_percent.c` into your project. There are no dependencies.

## API

| Function | Description |
| --- | --- |
| `battery_percent_from_millivolts(curve, millivolts)` | Charge percentage (0–100) of a single cell |
| `battery_percent_from_pack(curve, millivolts, cells)` | Charge percentage of a pack of identical cells in series |
| `battery_percent_adc_to_millivolts(raw, bits, reference_mv, r1, r2)` | Raw ADC reading to millivolts, including the voltage divider |
| `battery_percent_filter_init(filter, shift)` | Initialises a moving average filter with weight `1 / 2^shift` (shift up to 16) |
| `battery_percent_filter_update(filter, millivolts)` | Feeds a reading and returns the smoothed value |

### Custom curves

Measure your own cells for the best accuracy. Points must be sorted by increasing voltage.

```c
static const battery_percent_point_t points[] = {{3000, 0}, {3600, 20}, {3900, 80}, {4100, 100}};
static const battery_percent_curve_t my_cell = {points, sizeof(points) / sizeof(points[0])};

uint8_t percent = battery_percent_from_millivolts(&my_cell, 3750);
```

## Built-in curves

All curves are per cell, at room temperature.

| Curve | Range | Conditions | Source |
| --- | --- | --- | --- |
| `lipo` | 3306–4177 mV | Open-circuit voltage after 60 min rest, typical Li-ion polymer cell | [Maxim/ADI Application Note 4189](https://www.analog.com/en/design-notes/characterizing-a-lithiumion-li-cell-for-use-with-an-opencircuitvoltage-ocv-based-fuel-gauge.html), Table 1 (also used by Zephyr) |
| `li_ion` | 3281–4180 mV | Open-circuit voltage after 2 h rest, Samsung INR18650-20R (NMC), 0% at 2.5 V cutoff under 0.5C | [CALCE battery data](https://calce.umd.edu/battery-data), incremental OCV test |
| `lifepo4` | 2500–3500 mV | Average of C/20 charge and discharge, A123 cells | [CALCE battery data](https://calce.umd.edu/battery-data), low-current OCV test (Xing et al., Applied Energy 113, 2014) |
| `nimh` | 1000–1440 mV | Under 0.1C load, Energizer NH15-2300 AA | [Energizer NH15-2300 datasheet](https://data.energizer.com/pdfs/nh15-2300.pdf) |
| `alkaline` | 900–1500 mV | Under 50 mA intermittent load, 0% at 0.9 V, Energizer E91 AA | [Energizer E91 datasheet](https://data.energizer.com/pdfs/e91.pdf) |

LiFePO4 is nearly flat between 40% and 90% (about 50 mV), and NiMH between 45% and 80%: voltage can only resolve those
ranges coarsely.

## FAQ

### How do I measure battery voltage on an ESP32 or Arduino?

Connect the battery to an ADC pin through a voltage divider that keeps the pin below the ADC reference: two equal
100 kΩ resistors halve a 4.2 V LiPo to 2.1 V. Read the pin, then convert the value with
`battery_percent_adc_to_millivolts`, or on ESP32 use `analogReadMilliVolts()` multiplied by the divider ratio.

### Why does my battery percentage jump around?

Cell voltage sags while the device draws current (Wi-Fi transmissions, motors, displays), and ADCs are noisy. Smooth
the readings with `battery_percent_filter_update` and measure when the load is light.

### How accurate is voltage-based state of charge?

It is an estimate. It works best at rest and at room temperature, and degrades under heavy load, in the cold and on
aged cells. When you need precise gauging, use a fuel gauge IC such as the MAX17048 or BQ27441. `battery-percent` is
the cheap and portable option when all you have is an ADC pin.

### What voltage is 50% for a LiPo battery?

About 3.82 V at rest for a typical LiPo, and about 3.67 V for an 18650 NMC cell. The full tables are in
[`src/battery_percent.c`](https://github.com/AlixANNERAUD/battery-percent/blob/main/src/battery_percent.c).

## Development

```sh
cmake -S . -B build -DBATTERY_PERCENT_WERROR=ON -DBATTERY_PERCENT_SANITIZE=ON && cmake --build build && ctest --test-dir build
cargo test
pio run -d examples/platformio
python3 scripts/generate-chart.py
```

Releases are automated, see [RELEASING.md](https://github.com/AlixANNERAUD/battery-percent/blob/main/RELEASING.md).

## License

[MIT](https://github.com/AlixANNERAUD/battery-percent/blob/main/LICENSE)
