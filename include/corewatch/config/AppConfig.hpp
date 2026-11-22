#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace corewatch::config {

enum class ThemePreference {
    System,
    Light,
    Dark
};

struct AppConfig {
    std::uint32_t polling_interval_ms{1000};
    ThemePreference theme{ThemePreference::System};
    bool launch_at_startup{false};
    bool minimize_to_tray{true};
    bool alerts_enabled{true};
    double gpu_temp_threshold_c{85.0};
    double cpu_temp_threshold_c{90.0};
    std::uint32_t alert_cooldown_seconds{30};

    void sanitize() noexcept {
        polling_interval_ms = std::clamp(polling_interval_ms, 250U, 10000U);
        gpu_temp_threshold_c = std::clamp(gpu_temp_threshold_c, 30.0, 115.0);
        cpu_temp_threshold_c = std::clamp(cpu_temp_threshold_c, 30.0, 115.0);
        alert_cooldown_seconds = std::clamp(alert_cooldown_seconds, 5U, 3600U);
    }
};

} // namespace corewatch::config
