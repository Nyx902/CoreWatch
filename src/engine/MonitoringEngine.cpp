#include "corewatch/engine/MonitoringEngine.hpp"

#include <algorithm>

namespace corewatch::engine {

namespace {
constexpr std::size_t kMaxHistorySamples = 3600; // Covers 30m at >=500ms or 60m at 1000ms
constexpr std::size_t kMaxRecentAlerts = 50;
} // namespace

MonitoringEngine::MonitoringEngine(corewatch::config::AppConfig config)
    : config_(std::move(config))
    , history_buffer_(kMaxHistorySamples) {
    config_.sanitize();
}

MonitoringEngine::~MonitoringEngine() {
    stop();
}

void MonitoringEngine::register_provider(std::unique_ptr<corewatch::sensors::ISensorProvider> provider) {
    if (!provider) {
        return;
    }
    (void)provider->initialize();
    std::unique_lock lock(state_mutex_);
    providers_.push_back(std::move(provider));
}

void MonitoringEngine::start() {
    if (running_) {
        return;
    }
    running_ = true;
    worker_ = std::jthread([this](std::stop_token st) { run_loop(st); });
}

void MonitoringEngine::stop() {
    if (!running_) {
        return;
    }
    running_ = false;
    worker_.request_stop();
    wake_cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void MonitoringEngine::set_paused(bool paused) {
    {
        std::unique_lock lock(state_mutex_);
        paused_ = paused;
    }
    wake_cv_.notify_all();
}

bool MonitoringEngine::is_paused() const noexcept {
    std::shared_lock lock(state_mutex_);
    return paused_;
}

void MonitoringEngine::update_config(const corewatch::config::AppConfig& config) {
    {
        std::unique_lock lock(state_mutex_);
        config_ = config;
        config_.sanitize();
    }
    wake_cv_.notify_all();
}

corewatch::config::AppConfig MonitoringEngine::config() const {
    std::shared_lock lock(state_mutex_);
    return config_;
}

void MonitoringEngine::set_snapshot_callback(SnapshotCallback cb) {
    std::unique_lock lock(state_mutex_);
    snapshot_cb_ = std::move(cb);
}

void MonitoringEngine::set_alert_callback(AlertCallback cb) {
    std::unique_lock lock(state_mutex_);
    alert_cb_ = std::move(cb);
}

corewatch::core::SystemSnapshot MonitoringEngine::latest_snapshot() const {
    std::shared_lock lock(state_mutex_);
    return latest_snapshot_;
}

std::vector<corewatch::core::MetricSample> MonitoringEngine::history(corewatch::core::TimeWindow window) const {
    std::shared_lock lock(state_mutex_);
    auto all = history_buffer_.to_vector();
    const auto cutoff = std::chrono::steady_clock::now() - std::chrono::seconds(static_cast<int>(window));
    const auto it = std::lower_bound(
        all.begin(), all.end(), cutoff,
        [](const corewatch::core::MetricSample& sample, const std::chrono::steady_clock::time_point& tp) {
            return sample.timestamp < tp;
        });
    return std::vector<corewatch::core::MetricSample>(it, all.end());
}

std::vector<AlertEvent> MonitoringEngine::recent_alerts() const {
    std::shared_lock lock(state_mutex_);
    return recent_alerts_;
}

void MonitoringEngine::run_loop(std::stop_token stop_token) {
    while (!stop_token.stop_requested()) {
        bool should_poll = true;
        std::chrono::milliseconds interval{1000};
        {
            std::shared_lock lock(state_mutex_);
            should_poll = !paused_;
            interval = std::chrono::milliseconds(config_.polling_interval_ms);
        }

        if (should_poll) {
            poll_once();
        }

        std::unique_lock wake_lock(wake_mutex_);
        wake_cv_.wait_for(wake_lock, stop_token, interval, [] { return false; });
    }
}

void MonitoringEngine::poll_once() {
    corewatch::core::SystemSnapshot next{};
    next.timestamp = std::chrono::system_clock::now();

    corewatch::config::AppConfig cfg_copy{};
    SnapshotCallback snap_cb;
    AlertCallback alert_cb;

    {
        std::shared_lock lock(state_mutex_);
        for (const auto& provider : providers_) {
            if (provider) {
                provider->poll(next);
            }
        }
        cfg_copy = config_;
        snap_cb = snapshot_cb_;
        alert_cb = alert_cb_;
    }

    const auto now_steady = std::chrono::steady_clock::now();
    corewatch::core::MetricSample sample{};
    sample.timestamp = now_steady;
    sample.cpu_percent = next.cpu.total_utilization_percent.value.value_or(0.0);
    sample.memory_percent = next.memory.usage_percent.value.value_or(0.0);
    if (next.cpu.temperature_celsius.is_available()) {
        sample.cpu_temp_celsius = next.cpu.temperature_celsius.value;
    }
    if (!next.gpus.empty()) {
        if (next.gpus[0].utilization_percent.is_available()) {
            sample.gpu_percent = next.gpus[0].utilization_percent.value;
        }
        if (next.gpus[0].temperature_celsius.is_available()) {
            sample.gpu_temp_celsius = next.gpus[0].temperature_celsius.value;
        }
    }

    std::vector<AlertEvent> alerts;
    {
        std::unique_lock lock(state_mutex_);
        latest_snapshot_ = next;
        history_buffer_.push(sample);
        alerts = alert_evaluator_.evaluate(next, cfg_copy, now_steady);
        for (const auto& ev : alerts) {
            recent_alerts_.insert(recent_alerts_.begin(), ev);
            if (recent_alerts_.size() > kMaxRecentAlerts) {
                recent_alerts_.pop_back();
            }
        }
    }

    if (snap_cb) {
        snap_cb(next);
    }
    if (alert_cb) {
        for (const auto& ev : alerts) {
            alert_cb(ev);
        }
    }
}

} // namespace corewatch::engine
