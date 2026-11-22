# CoreWatch

**CoreWatch** is a lightweight, strictly read-only, native Windows 11 hardware monitoring application built with **C++20**, **WinUI 3 / C++/WinRT**, **Windows App SDK**, **Win32/PDH APIs**, and **NVIDIA NVML**.

## Features

- **Native Fluent Design UI**: Built with WinUI 3, Mica backdrop, `NavigationView` sidebar, `Segoe UI Variable`, Fluent icons, and Light / Dark / System theme support.
- **Dashboard**: Real-time summary cards for CPU, GPU, RAM, and Storage alongside live time-series charts.
- **CPU Monitoring**: Total utilization (`GetSystemTimes`), per-logical-processor utilization (PDH), and clock frequency (`CallNtPowerInformation`). Temperature and power are displayed as `Unavailable` unless provided by a supported unprivileged sensor.
- **NVIDIA GPU Monitoring**: Dynamically loads `nvml.dll` via `LoadLibraryW` / `GetProcAddress` when present—displaying GPU utilization, temperature, VRAM usage, power draw/limit, fan speed, and core/memory clocks, while gracefully handling missing or unsupported sensors.
- **Memory & Storage**: Physical RAM, commit charge, fixed-volume capacity, disk active time, and read/write throughput.
- **Native Live Charts**: Bounded in-memory `RingBuffer` supporting `1 minute`, `5 minutes`, and `30 minutes` views using native XAML `Canvas` + `Polyline`/`Polygon` (zero WebView2 or JavaScript runtime).
- **Temperature Alerts**: Configurable thresholds with hysteresis and cooldown, surfacing tray balloon notifications and an in-app event log.
- **System Tray & Persistence**: Background monitoring, minimize-to-tray, pause/resume, JSON configuration at `%LOCALAPPDATA%\CoreWatch\config.json`, and optional Windows startup integration.

## Requirements

- Windows 10 (19041+) or Windows 11 (recommended)
- x64 or ARM64 processor
- Optional: NVIDIA display driver (`nvml.dll`) for NVIDIA GPU telemetry
- For building from source:
  - Visual Studio 2022 (or newer) with **Desktop development with C++** and **Windows App SDK C++ templates**
  - Windows 10/11 SDK (`10.0.22621.0` or newer)
  - CMake 3.24+

## Installation

Download the latest release archive from the Releases page (or build from source below) and run `CoreWatch.App.exe`. No installer, kernel driver, or administrator privileges are required.

## Building from Source

### 1. Build & Run Native Unit Tests (CMake)

```powershell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### 2. Build the WinUI 3 Application (MSBuild)

```powershell
msbuild CoreWatch.sln /t:Restore /p:Configuration=Release /p:Platform=x64
msbuild CoreWatch.sln /p:Configuration=Release /p:Platform=x64 -m
```

The compiled executable and self-contained Windows App SDK runtime files will be placed in:
`out\x64\Release\CoreWatch.App\CoreWatch.App.exe`

### Release Build Instructions

To produce a clean release package:
1. Run `ctest --test-dir build --output-on-failure` to verify all unit tests pass.
2. Build `CoreWatch.sln` in `Release|x64` (and optionally `Release|ARM64`).
3. Archive the contents of `out\x64\Release\CoreWatch.App\` (excluding `.pdb` / `.lib` / `.exp` if desired).

## Architecture

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for a detailed breakdown of the 5-layer architecture, threading model, and RAII resource management.

## Known Limitations

- **CPU Temperature & Package Power**: Standard unprivileged Windows user-mode APIs do not expose CPU die temperature or package power without a kernel driver. Because CoreWatch is strictly driverless and read-only, these fields display `Unavailable`.
- **Non-NVIDIA GPUs**: Currently only NVIDIA GPUs are queried via `nvml.dll`; AMD/Intel GPU sensor providers are not yet implemented.
- **Unpackaged Deployment**: Configured with `WindowsPackageType=None` and `WindowsAppSDKSelfContained=true` for frictionless portable execution without MSIX signing.

## License

This project is licensed under the [MIT License](LICENSE).
