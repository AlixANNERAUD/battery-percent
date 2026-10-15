# battery-percent

Hardware-agnostic battery state-of-charge estimation for embedded systems.

You measure the voltage however your platform allows. The library turns it into a percentage, using per-chemistry discharge curves with linear interpolation between points.

- Plain C99, no allocation, no dependencies, no platform headers
- Usable from C, C++, Rust ([crates.io](https://crates.io/crates/battery-percent)) and any language with a C FFI
- Built-in curves: LiPo, Li-ion, LiFePO4, NiMH, alkaline
- Custom curves, multi-cell packs, ADC-to-millivolts helper, optional smoothing filter

## C / C++

```c
#include "battery_percent.h"

uint32_t raw = read_adc_somehow();
uint32_t millivolts = battery_percent_adc_to_millivolts(raw, 12, 3300, 100000, 100000);

uint8_t percent = battery_percent_from_millivolts(&battery_percent_curve_lipo, (uint16_t)millivolts);
uint8_t pack = battery_percent_from_pack(&battery_percent_curve_li_ion, 11100, 3);
```

If your platform already returns calibrated millivolts (for example `analogReadMilliVolts()` on ESP32), skip `battery_percent_adc_to_millivolts` and pass the value directly. Remember to multiply by your voltage divider ratio.

### API

| Function | Description |
| --- | --- |
| `battery_percent_from_millivolts(curve, millivolts)` | Charge percentage (0–100) of a single cell |
| `battery_percent_from_pack(curve, millivolts, cells)` | Charge percentage of a pack of identical cells in series |
| `battery_percent_adc_to_millivolts(raw, bits, reference_mv, r1, r2)` | Converts a raw ADC reading to millivolts. `r1` is the resistor between battery and ADC pin, `r2` the one between ADC pin and ground. Pass `r2 = 0` without a divider |
| `battery_percent_filter_init(filter, shift)` | Initialises an exponential moving average filter with weight `1 / 2^shift` (shift up to 16) |
| `battery_percent_filter_update(filter, millivolts)` | Feeds a reading and returns the smoothed value |

### Custom curves

Points must be sorted by increasing voltage.

```c
static const battery_percent_point_t points[] = {{3000, 0}, {3600, 20}, {3900, 80}, {4100, 100}};
static const battery_percent_curve_t curve = {points, sizeof(points) / sizeof(points[0])};

uint8_t percent = battery_percent_from_millivolts(&curve, 3750);
```

### Integration

- **PlatformIO**: `lib_deps = alix-anneraud/battery-percent`
- **CMake** (ESP-IDF, Zephyr, desktop…): `add_subdirectory(battery-percent)` and link `battery_percent::battery_percent`
- **Anything else**: compile `src/battery_percent.c` and add `include/` to your include path

## Rust

```toml
[dependencies]
battery-percent = "2"
```

```rust
use battery_percent::{adc_to_millivolts, Curve, Filter, Point};

let millivolts = adc_to_millivolts(2383, 12, 3300, 100_000, 100_000);
let percent = Curve::lipo().percent(millivolts as u16);
assert!(percent <= 100);

let mut filter = Filter::new(3);
let smoothed = filter.update(millivolts as u16);
assert_eq!(smoothed, millivolts as u16);

const POINTS: [Point; 3] = [Point::new(3000, 0), Point::new(3700, 50), Point::new(4200, 100)];
let custom = Curve::new(&POINTS);
assert_eq!(custom.percent(3700), 50);
assert_eq!(custom.pack_percent(7400, 2), 50);
```

The crate is `no_std`. It compiles the C core with the [`cc`](https://crates.io/crates/cc) crate, so cross-compiling needs a C compiler for the target (e.g. `arm-none-eabi-gcc`). Raw bindings are available in `battery_percent::ffi`.

## Built-in curves

All curves are per cell, at room temperature.

| Curve | Range | Conditions | Source |
| --- | --- | --- | --- |
| `lipo` | 3306–4177 mV | Open-circuit voltage after 60 min rest, typical Li-ion polymer cell | [Maxim/ADI Application Note 4189](https://www.analog.com/en/design-notes/characterizing-a-lithiumion-li-cell-for-use-with-an-opencircuitvoltage-ocv-based-fuel-gauge.html), Table 1 (also used by Zephyr) |
| `li_ion` | 3281–4180 mV | Open-circuit voltage after 2 h rest, Samsung INR18650-20R (NMC), 0% at 2.5 V cutoff under 0.5C | [CALCE battery data](https://calce.umd.edu/battery-data), incremental OCV test |
| `lifepo4` | 2500–3500 mV | Average of C/20 charge and discharge, A123 cells | [CALCE battery data](https://calce.umd.edu/battery-data), low-current OCV test (Xing et al., Applied Energy 113, 2014) |
| `nimh` | 1000–1440 mV | Under 0.1C load, Energizer NH15-2300 AA | [Energizer NH15-2300 datasheet](https://data.energizer.com/pdfs/nh15-2300.pdf) |
| `alkaline` | 900–1500 mV | Under 50 mA intermittent load, 0% at 0.9 V, Energizer E91 AA | [Energizer E91 datasheet](https://data.energizer.com/pdfs/e91.pdf) |

LiFePO4 is nearly flat between 40% and 90% (about 50 mV), and NiMH between 45% and 80%: voltage can only resolve those ranges coarsely.

## Accuracy

Voltage-based estimation is an approximation. Cell voltage drops under load and varies with temperature and aging. For best results measure while the load is light, use the filter, and provide a curve measured on your own cells. For precise gauging, use a dedicated fuel gauge IC (MAX17048, BQ27441…).

## Testing

```sh
cmake -S . -B build -DBATTERY_PERCENT_WERROR=ON -DBATTERY_PERCENT_SANITIZE=ON && cmake --build build && ctest --test-dir build
cargo test
pio run -d examples/platformio
```

## Releasing

See [RELEASING.md](RELEASING.md).

## License

MIT
