# Contributing to CoreWatch

Thank you for your interest in contributing to **CoreWatch**!

## Design Principles

1. **Read-only safety**: CoreWatch must never modify voltages, fan curves, clock speeds, power limits, or BIOS settings, and must never require administrator privileges or install kernel drivers.
2. **Honest telemetry**: Never fabricate sensor readings. If a sensor is unsupported or inaccessible, return `SensorValue::unavailable()` or `SensorValue::unsupported()` so the UI displays `Unavailable`.
3. **Off-UI polling**: All hardware queries must execute on background worker threads, never on the WinUI 3 UI thread.
4. **Modern C++20**: Prefer RAII wrappers, smart pointers, `std::chrono`, `std::jthread`, and `<format>`.

## Building & Testing Locally

### Native Libraries & Unit Tests (CMake)

```powershell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### WinUI 3 Application (MSBuild)

```powershell
msbuild CoreWatch.sln /t:Restore /p:Configuration=Release /p:Platform=x64
msbuild CoreWatch.sln /p:Configuration=Release /p:Platform=x64 -m
```

## Pull Request Checklist

- Ensure all CMake unit tests pass (`ctest --test-dir build --output-on-failure`).
- Ensure the WinUI 3 solution builds cleanly in `Release|x64`.
- Keep changes focused and well-documented.
