# Changelog

All notable changes to **CoreWatch** will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-11-22

### Added
- Native WinUI 3 + C++/WinRT Fluent Design interface with Mica backdrop, NavigationView sidebar, and Light/Dark/System themes.
- `corewatch_core`: Shared telemetry models (`SensorValue<T>`, `SystemSnapshot`), bounded `RingBuffer<T>`, and unit formatting utilities.
- `corewatch_sensors`: Common `ISensorProvider` interface with Win32 CPU (`GetSystemTimes`, PDH per-core, `CallNtPowerInformation`), Memory (`GlobalMemoryStatusEx`), Storage (`GetDiskFreeSpaceExW`, PDH disk throughput), and dynamic NVIDIA NVML (`LoadLibraryW("nvml.dll")`) GPU provider.
- `corewatch_engine`: Background `std::jthread` polling engine with `1m` / `5m` / `30m` history queries and `AlertEvaluator` with hysteresis and cooldown.
- `corewatch_config`: Zero-dependency C++20 JSON config persistence (`%LOCALAPPDATA%\CoreWatch\config.json`) and Windows `HKCU\...\Run` startup toggle.
- Native XAML `LiveChartControl` (`Canvas` + `Polyline` + `Polygon`) with zero web runtime.
- System tray integration (`Shell_NotifyIconW`) with minimize-to-tray, pause/resume, balloon alerts, and exit.
- CMake unit test suite (`corewatch_tests`) and GitHub Actions Windows CI workflow.
