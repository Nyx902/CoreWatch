#pragma once

#include "corewatch/config/AppConfig.hpp"
#include "corewatch/core/RingBuffer.hpp"
#include "corewatch/core/Types.hpp"
#include "corewatch/engine/AlertEvaluator.hpp"
#include "corewatch/sensors/ISensorProvider.hpp"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stop_token>
#include <thread>
#include <vector>

namespace corewatch::engine {

class MonitoringEngine {
public:
    using SnapshotCallback = std::function<void(const corewatch::core::SystemSnapshot&)>;
    using AlertCallback = std::function<void(const AlertEvent&)>;

    explicit MonitoringEngine(corewatch::config::AppConfig config = {});
    ~MonitoringEngine();

    MonitoringEngine(const MonitoringEngine&) = delete;
    MonitoringEngine& operator=(const MonitoringEngine&) = delete;

    void register_provider(std::unique_ptr<corewatch::sensors::ISensorProvider> provider);

    void start();
    void stop();
    void set_paused(bool paused);
    [[nodiscard]] bool is_paused() const noexcept;

    void update_config(const corewatch::config::AppConfig& config);
    [[nodiscard]] corewatch::config::AppConfig config() const;

    void set_snapshot_callback(SnapshotCallback cb);
    void set_alert_callback(AlertCallback cb);

    [[nodiscard]] corewatch::core::SystemSnapshot latest_snapshot() const;
    [[nodiscard]] std::vector<corewatch::core::MetricSample> history(corewatch::core::TimeWindow window) const;
    [[nodiscard]] std::vector<AlertEvent> recent_alerts() const;

private:
    void run_loop(std::stop_token stop_token);
    void poll_once();

    mutable std::shared_mutex state_mutex_;
    std::mutex wake_mutex_;
    std::condition_variable_any wake_cv_;

    std::vector<std::unique_ptr<corewatch::sensors::ISensorProvider>> providers_;
    corewatch::config::AppConfig config_{};
    corewatch::core::SystemSnapshot latest_snapshot_{};
    corewatch::core::RingBuffer<corewatch::core::MetricSample> history_buffer_;
    AlertEvaluator alert_evaluator_{};
    std::vector<AlertEvent> recent_alerts_;

    SnapshotCallback snapshot_cb_;
    AlertCallback alert_cb_;
    bool paused_{false};
    bool running_{false};
    std::jthread worker_;
};

} // namespace corewatch::engine
