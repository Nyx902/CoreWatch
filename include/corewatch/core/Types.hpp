#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace corewatch::core {

enum class SensorAvailability {
    Available,
    Unavailable,
    Unsupported
};

template <typename T>
struct SensorValue {
    std::optional<T> value{};
    SensorAvailability availability{SensorAvailability::Unavailable};

    [[nodiscard]] constexpr bool is_available() const noexcept {
        return availability == SensorAvailability::Available && value.has_value();
    }

    [[nodiscard]] static constexpr SensorValue<T> available(T v) noexcept {
        return SensorValue<T>{std::move(v), SensorAvailability::Available};
    }

    [[nodiscard]] static constexpr SensorValue<T> unavailable() noexcept {
        return SensorValue<T>{std::nullopt, SensorAvailability::Unavailable};
    }

    [[nodiscard]] static constexpr SensorValue<T> unsupported() noexcept {
        return SensorValue<T>{std::nullopt, SensorAvailability::Unsupported};
    }
};

struct LogicalCoreReading {
    std::uint32_t core_index{0};
    SensorValue<double> utilization_percent{};
};

struct CpuSnapshot {
    std::string name{"Unknown CPU"};
    std::uint32_t logical_core_count{0};
    std::uint32_t physical_core_count{0};
    SensorValue<double> total_utilization_percent{};
    SensorValue<double> current_clock_mhz{};
    SensorValue<double> max_clock_mhz{};
    SensorValue<double> temperature_celsius{};
    SensorValue<double> power_watts{};
    std::vector<LogicalCoreReading> logical_cores{};
};

struct GpuSnapshot {
    std::uint32_t index{0};
    std::string name{"Unavailable"};
    std::string vendor{"Unknown"};
    std::string driver_version{};
    bool nvml_active{false};
    SensorValue<double> utilization_percent{};
    SensorValue<double> temperature_celsius{};
    SensorValue<std::uint64_t> vram_used_bytes{};
    SensorValue<std::uint64_t> vram_total_bytes{};
    SensorValue<double> power_watts{};
    SensorValue<double> power_limit_watts{};
    SensorValue<double> fan_speed_percent{};
    SensorValue<double> graphics_clock_mhz{};
    SensorValue<double> memory_clock_mhz{};
};

struct MemorySnapshot {
    SensorValue<std::uint64_t> total_bytes{};
    SensorValue<std::uint64_t> used_bytes{};
    SensorValue<std::uint64_t> available_bytes{};
    SensorValue<double> usage_percent{};
    SensorValue<std::uint64_t> commit_total_bytes{};
    SensorValue<std::uint64_t> commit_limit_bytes{};
};

struct StorageVolumeSnapshot {
    std::string mount_point{};   // e.g. "C:\\"
    std::string volume_label{};  // e.g. "Windows"
    std::string file_system{};   // e.g. "NTFS"
    SensorValue<std::uint64_t> total_bytes{};
    SensorValue<std::uint64_t> used_bytes{};
    SensorValue<std::uint64_t> free_bytes{};
    SensorValue<double> usage_percent{};
};

struct StorageSnapshot {
    std::vector<StorageVolumeSnapshot> volumes{};
    SensorValue<double> disk_active_time_percent{};
    SensorValue<double> read_bytes_per_sec{};
    SensorValue<double> write_bytes_per_sec{};
};

struct SystemSnapshot {
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    CpuSnapshot cpu{};
    std::vector<GpuSnapshot> gpus{};
    MemorySnapshot memory{};
    StorageSnapshot storage{};
};

enum class TimeWindow {
    OneMinute = 60,
    FiveMinutes = 300,
    ThirtyMinutes = 1800
};

struct MetricSample {
    std::chrono::steady_clock::time_point timestamp{};
    double cpu_percent{0.0};
    double memory_percent{0.0};
    std::optional<double> gpu_percent{};
    std::optional<double> gpu_temp_celsius{};
    std::optional<double> cpu_temp_celsius{};
};

} // namespace corewatch::core
