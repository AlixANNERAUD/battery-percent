# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.1](https://github.com/AlixANNERAUD/battery-percent/compare/v2.0.0...v2.0.1) - 2026-10-15

### Documentation

- make the readme and package metadata attractive for new users
- clean up the 2.0.0 changelog entry

## [2.0.0](https://github.com/AlixANNERAUD/battery-percent/releases/tag/v2.0.0) - 2026-10-15

Renamed from Battery Library: the package is now `battery-percent` on PlatformIO and crates.io.

### Added

- Hardware-agnostic plain C99 core: the caller provides a voltage in millivolts and gets a charge percentage.
- Sourced discharge curves with interpolation for LiPo, Li-ion 18650, LiFePO4, NiMH and alkaline cells.
- Custom curves, multi-cell series packs, raw ADC to millivolts conversion with a voltage divider, and an integer
  moving average filter.
- `no_std` Rust crate on crates.io wrapping the C core.
- PlatformIO support for any platform and framework, and a CMake target.

### Changed

- Header renamed to `battery_percent.h`, with every symbol prefixed by `battery_percent_`.

### Removed

- The Arduino `Battery_Class` API and its linear mapping between a minimum and a maximum voltage.

## [1.0.3] - 2023-06-04

- Remove log messages.

## [1.0.2] - 2023-06-04

- Increase precision of measurements (changing calculation methods).

## [1.0.1] - 2021-05-21

- Increase precision of measurements (changing calculation methods).

## [1.0.0] - 2021-04-15

- Initial version.

