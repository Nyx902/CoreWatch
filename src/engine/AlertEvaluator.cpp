#include "corewatch/engine/AlertEvaluator.hpp"

#include <format>

namespace corewatch::engine {

std::vector<AlertEvent> AlertEvaluator::evaluate(
    const corewatch::core::SystemSnapshot& snapshot,
    const corewatch::config::AppConfig& config,
    std::chrono::steady_clock::time_point now) {
    std::vector<AlertEvent> triggered;
    if (!config.alerts_enabled) {
        return triggered;
    }

    const auto cooldown = std::chrono::seconds(config.alert_cooldown_seconds);

    auto check_sensor = [&](const std::string& id,
                            const std::string& label,
                            const corewatch::core::SensorValue<double>& reading,
                            double threshold_c) {
        if (!reading.is_available()) {
            return;
        }
        const double temp = *reading.value;
        auto& state = states_[id];

        if (temp >= threshold_c) {
            const bool cooldown_elapsed =
                (state.last_triggered == std::chrono::steady_clock::time_point{}) ||
                ((now - state.last_triggered) >= cooldown);

            if (!state.active || cooldown_elapsed) {
                state.active = true;
                state.last_triggered = now;
                triggered.push_back(AlertEvent{
                    id,
                    std::format("{} Temperature Alert", label),
                    std::format("{} reached {:.1f} \xC2\xB0""C (threshold {:.1f} \xC2\xB0""C)",
                                label, temp, threshold_c),
                    temp,
                    threshold_c,
                    snapshot.timestamp
                });
            }
        } else if (temp <= (threshold_c - kHysteresisCelsius)) {
            state.active = false;
        }
    };

    check_sensor("cpu_temp", "CPU", snapshot.cpu.temperature_celsius, config.cpu_temp_threshold_c);

    for (const auto& gpu : snapshot.gpus) {
        const std::string id = std::format("gpu_{}_temp", gpu.index);
        const std::string label = gpu.name.empty() ? std::format("GPU {}", gpu.index) : gpu.name;
        check_sensor(id, label, gpu.temperature_celsius, config.gpu_temp_threshold_c);
    }

    return triggered;
}

void AlertEvaluator::reset() noexcept {
    states_.clear();
}

} // namespace corewatch::engine
