#pragma once

#include "corewatch/config/AppConfig.hpp"
#include "corewatch/core/Types.hpp"

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace corewatch::engine {

struct AlertEvent {
    std::string sensor_id;
    std::string title;
    std::string message;
    double value_celsius{0.0};
    double threshold_celsius{0.0};
    std::chrono::system_clock::time_point timestamp{};
};

class AlertEvaluator {
public:
    [[nodiscard]] std::vector<AlertEvent> evaluate(
        const corewatch::core::SystemSnapshot& snapshot,
        const corewatch::config::AppConfig& config,
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

    void reset() noexcept;

private:
    static constexpr double kHysteresisCelsius = 3.0;

    struct SensorState {
        bool active{false};
        std::chrono::steady_clock::time_point last_triggered{};
    };

    std::unordered_map<std::string, SensorState> states_;
};

} // namespace corewatch::engine
