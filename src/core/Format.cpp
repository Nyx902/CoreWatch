#include "corewatch/core/Format.hpp"

#include <array>
#include <cstdio>
#include <format>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace corewatch::core {

std::string format_bytes(std::uint64_t bytes) {
    constexpr std::array<const char*, 5> units{"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    std::size_t unit_index = 0;
    while (value >= 1024.0 && unit_index + 1 < units.size()) {
        value /= 1024.0;
        ++unit_index;
    }
    if (unit_index == 0) {
        return std::format("{} {}", bytes, units[0]);
    }
    return std::format("{:.1f} {}", value, units[unit_index]);
}

std::string format_bytes_per_sec(double bytes_per_sec) {
    if (bytes_per_sec < 0.0) {
        bytes_per_sec = 0.0;
    }
    constexpr std::array<const char*, 4> units{"B/s", "KB/s", "MB/s", "GB/s"};
    double value = bytes_per_sec;
    std::size_t unit_index = 0;
    while (value >= 1024.0 && unit_index + 1 < units.size()) {
        value /= 1024.0;
        ++unit_index;
    }
    return std::format("{:.1f} {}", value, units[unit_index]);
}

std::string format_percent(const SensorValue<double>& value) {
    if (!value.is_available()) {
        return "Unavailable";
    }
    return std::format("{:.1f}%", *value.value);
}

std::string format_temperature_c(const SensorValue<double>& value) {
    if (!value.is_available()) {
        return "Unavailable";
    }
    return std::format("{:.1f} \xC2\xB0""C", *value.value);
}

std::string format_watts(const SensorValue<double>& value) {
    if (!value.is_available()) {
        return "Unavailable";
    }
    return std::format("{:.1f} W", *value.value);
}

std::string format_clock_mhz(const SensorValue<double>& value) {
    if (!value.is_available()) {
        return "Unavailable";
    }
    const double mhz = *value.value;
    if (mhz >= 1000.0) {
        return std::format("{:.2f} GHz", mhz / 1000.0);
    }
    return std::format("{:.0f} MHz", mhz);
}

std::string format_bytes_sensor(const SensorValue<std::uint64_t>& value) {
    if (!value.is_available()) {
        return "Unavailable";
    }
    return format_bytes(*value.value);
}

std::wstring utf8_to_wide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int needed = MultiByteToWideChar(
        CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (needed <= 0) {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(needed), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), needed);
    return wide;
}

std::string wide_to_utf8(const std::wstring& wide) {
    if (wide.empty()) {
        return {};
    }
    const int needed = WideCharToMultiByte(
        CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return {};
    }
    std::string utf8(static_cast<std::size_t>(needed), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), needed, nullptr, nullptr);
    return utf8;
}

} // namespace corewatch::core
