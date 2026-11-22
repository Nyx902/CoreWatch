# CoreWatch Architecture

CoreWatch is structured as five decoupled C++20 layers so that all hardware polling, time-series buffering, alert evaluation, and configuration persistence are independent of the WinUI 3 presentation layer.

## Layer Overview

```
+-------------------------------------------------------------+
|  WinUI 3 + C++/WinRT Application (app/CoreWatch.App)        |
|  - NavigationView Pages (Dashboard, CPU, GPU, Mem/Disk, ...) |
|  - Native XAML LiveChartControl (Canvas + Polyline/Polygon) |
|  - TrayIconManager (Shell_NotifyIconW)                      |
+------------------------------+------------------------------+
                               | DispatcherQueue.TryEnqueue()
                               v
+-------------------------------------------------------------+
|  Monitoring Engine (corewatch_engine)                       |
|  - std::jthread background polling loop + stop_token        |
|  - Bounded RingBuffer<MetricSample> (1m / 5m / 30m)         |
|  - AlertEvaluator (hysteresis + cooldown)                   |
+--------------+-------------------------------+--------------+
               |                               |
               v                               v
+------------------------------+ +----------------------------+
| Sensor Providers             | | Config & Persistence       |
| (corewatch_sensors)          | | (corewatch_config)         |
| - ISensorProvider            | | - AppConfig                |
| - CpuSensorProvider          | | - JsonSerializer           |
| - MemorySensorProvider       | | - ConfigStore              |
| - StorageSensorProvider      | | - HKCU Run startup toggle  |
| - NvmlGpuSensorProvider      | +----------------------------+
+--------------+---------------+
               |
               v
+-------------------------------------------------------------+
|  Shared Models & Utilities (corewatch_core)                 |
|  - SensorValue<T> (Available / Unavailable / Unsupported)   |
|  - SystemSnapshot, RingBuffer<T>, Format helpers            |
+-------------------------------------------------------------+
```

## Threading & Safety Model

- **Zero UI-thread hardware polling**: `MonitoringEngine` owns a `std::jthread` that wakes on the configured `polling_interval_ms` (or immediately on `stop()` / `update_config()` via `std::condition_variable_any`).
- **Thread-safe state**: `MonitoringEngine` guards `latest_snapshot_`, `history_buffer_`, and `config_` with `std::shared_mutex`.
- **UI marshaling**: Snapshot and alert callbacks enqueue updates onto the WinUI 3 `DispatcherQueue`.
- **RAII resource management**: Win32 `PDH_HQUERY` handles and dynamically loaded `nvml.dll` `HMODULE` handles are wrapped in `std::unique_ptr` with custom deleters; `nvmlShutdown()` is guaranteed in `Impl::~Impl()`.
- **Strict read-only posture**: No kernel drivers, no elevation/admin prompts, and no write operations to hardware registers or power/fan controls.
