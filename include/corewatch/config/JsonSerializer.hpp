#pragma once

#include "corewatch/config/AppConfig.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace corewatch::config {

/// Serializes AppConfig to formatted JSON.
[[nodiscard]] std::string serialize_config_json(const AppConfig& config);

/// Deserializes AppConfig from JSON. Missing fields keep default values.
/// Returns std::nullopt if the input is not a valid JSON object.
[[nodiscard]] std::optional<AppConfig> deserialize_config_json(std::string_view json);

} // namespace corewatch::config
