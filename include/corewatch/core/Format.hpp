#pragma once

#include "corewatch/core/Types.hpp"

#include <cstdint>
#include <string>

namespace corewatch::core {

/// Formats a byte count into a human-readable string (e.g., "16.4 GB").
[[nodiscard]] std::string format_bytes(std::uint64_t bytes);

/// Formats a throughput in bytes/sec (e.g., "42.1 MB/s").
[[nodiscard]] std::string format_bytes_per_sec(double bytes_per_sec);

/// Formats a percentage value or returns "Unavailable".
[[nodiscard]] std::string format_percent(const SensorValue<double>& value);

/// Formats temperature in Celsius or returns "Unavailable".
[[nodiscard]] std::string format_temperature_c(const SensorValue<double>& value);

/// Formats power in Watts or returns "Unavailable".
[[nodiscard]] std::string format_watts(const SensorValue<double>& value);

/// Formats clock frequency in MHz / GHz or returns "Unavailable".
[[nodiscard]] std::string format_clock_mhz(const SensorValue<double>& value);

/// Formats a byte sensor value or returns "Unavailable".
[[nodiscard]] std::string format_bytes_sensor(const SensorValue<std::uint64_t>& value);

/// Converts UTF-8 std::string to UTF-16 std::wstring.
[[nodiscard]] std::wstring utf8_to_wide(const std::string& utf8);

/// Converts UTF-16 std::wstring to UTF-8 std::string.
[[nodiscard]] std::string wide_to_utf8(const std::wstring& wide);

} // namespace corewatch::core
