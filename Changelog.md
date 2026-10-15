# Changelog

All notable changes to this project will be documented in this file.

This project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - Unreleased

- Rename to `battery-percent` (header `battery_percent.h`, prefix `battery_percent_`).
- Rewrite as a hardware-agnostic plain C99 library: the caller provides the voltage, the library returns the percentage.
- Replace linear mapping with per-chemistry discharge curves and interpolation (LiPo, Li-ion, LiFePO4, NiMH, alkaline).
- Add custom curves, multi-cell packs, ADC-to-millivolts conversion and an exponential moving average filter.
- Add Rust bindings published on crates.io as `battery-percent`.
- Support any PlatformIO platform and framework, and CMake.
- Remove the Arduino `Battery_Class` API.

## [1.0.3] - 2023-06-04

- Remove log messages.

## [1.0.2] - 2023-06-04

- Increase precision of measurements (changing calculation methods).

## [1.0.1] - 2021-05-21

- Increase precision of measurements (changing calculation methods).

## [1.0.0] - 2021-04-15

- Initial version.

