#include "TestHarness.hpp"

#include "corewatch/config/AppConfig.hpp"
#include "corewatch/config/ConfigStore.hpp"
#include "corewatch/config/JsonSerializer.hpp"
#include "corewatch/core/Format.hpp"
#include "corewatch/core/RingBuffer.hpp"
#include "corewatch/core/Types.hpp"
#include "corewatch/engine/AlertEvaluator.hpp"
#include "corewatch/engine/MonitoringEngine.hpp"
#include "corewatch/sensors/CpuSensorProvider.hpp"
#include "corewatch/sensors/MemorySensorProvider.hpp"
#include "corewatch/sensors/NvmlGpuSensorProvider.hpp"
#include "corewatch/sensors/StorageSensorProvider.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

using namespace corewatch::core;
using namespace corewatch::config;
using namespace corewatch::engine;
using namespace corewatch::sensors;

CW_TEST(RingBufferOverwriteAndOrder) {
    RingBuffer<int> buf(3);
    CW_EXPECT_TRUE(buf.empty());
    buf.push(10);
    buf.push(20);
    buf.push(30);
    CW_EXPECT_TRUE(buf.full());
    CW_EXPECT_EQ(buf.size(), std::size_t{3});

    buf.push(40);
    auto vec = buf.to_vector();
    CW_EXPECT_EQ(vec.size(), std::size_t{3});
    CW_EXPECT_EQ(vec[0], 20);
    CW_EXPECT_EQ(vec[1], 30);
    CW_EXPECT_EQ(vec[2], 40);

    auto last2 = buf.tail(2);
    CW_EXPECT_EQ(last2.size(), std::size_t{2});
    CW_EXPECT_EQ(last2[0], 30);
    CW_EXPECT_EQ(last2[1], 40);
}

CW_TEST(FormattingUnavailableAndUnits) {
    CW_EXPECT_EQ(format_percent(SensorValue<double>::unavailable()), std::string("Unavailable"));
    CW_EXPECT_EQ(format_temperature_c(SensorValue<double>::unsupported()), std::string("Unavailable"));
    CW_EXPECT_EQ(format_watts(SensorValue<double>::unavailable()), std::string("Unavailable"));
    CW_EXPECT_EQ(format_clock_mhz(SensorValue<double>::available(3650.0)), std::string("3.65 GHz"));
    CW_EXPECT_EQ(format_clock_mhz(SensorValue<double>::available(850.0)), std::string("850 MHz"));
    CW_EXPECT_EQ(format_bytes(1024ULL * 1024ULL * 1024ULL), std::string("1.0 GB"));
}

CW_TEST(JsonConfigRoundTripAndSanitization) {
    AppConfig cfg{};
    cfg.polling_interval_ms = 500;
    cfg.theme = ThemePreference::Dark;
    cfg.launch_at_startup = true;
    cfg.minimize_to_tray = false;
    cfg.alerts_enabled = true;
    cfg.gpu_temp_threshold_c = 82.5;
    cfg.cpu_temp_threshold_c = 91.0;
    cfg.alert_cooldown_seconds = 45;

    const std::string json = serialize_config_json(cfg);
    auto parsed = deserialize_config_json(json);
    CW_EXPECT_TRUE(parsed.has_value());
    CW_EXPECT_EQ(parsed->polling_interval_ms, 500U);
    CW_EXPECT_TRUE(parsed->theme == ThemePreference::Dark);
    CW_EXPECT_TRUE(parsed->launch_at_startup);
    CW_EXPECT_TRUE(!parsed->minimize_to_tray);
    CW_EXPECT_EQ(parsed->gpu_temp_threshold_c, 82.5);
    CW_EXPECT_EQ(parsed->alert_cooldown_seconds, 45U);

    // Invalid JSON returns nullopt
    CW_EXPECT_TRUE(!deserialize_config_json("not json").has_value());
}

CW_TEST(ConfigStorePersistence) {
    const auto temp_path = std::filesystem::temp_directory_path() / "corewatch_test_cfg.json";
    std::error_code ec;
    std::filesystem::remove(temp_path, ec);

    ConfigStore store(temp_path);
    AppConfig cfg{};
    cfg.polling_interval_ms = 2000;
    cfg.theme = ThemePreference::Light;
    CW_EXPECT_TRUE(store.save(cfg));

    const AppConfig loaded = store.load();
    CW_EXPECT_EQ(loaded.polling_interval_ms, 2000U);
    CW_EXPECT_TRUE(loaded.theme == ThemePreference::Light);

    std::filesystem::remove(temp_path, ec);
}

CW_TEST(AlertEvaluatorHysteresisAndCooldown) {
    AlertEvaluator evaluator;
    AppConfig cfg{};
    cfg.alerts_enabled = true;
    cfg.gpu_temp_threshold_c = 80.0;
    cfg.alert_cooldown_seconds = 30;

    SystemSnapshot snap{};
    GpuSnapshot gpu{};
    gpu.index = 0;
    gpu.name = "Test GPU";
    gpu.temperature_celsius = SensorValue<double>::available(82.0);
    snap.gpus.push_back(gpu);

    const auto t0 = std::chrono::steady_clock::now();
    auto alerts1 = evaluator.evaluate(snap, cfg, t0);
    CW_EXPECT_EQ(alerts1.size(), std::size_t{1});

    // Within cooldown: should not re-trigger immediately
    auto alerts2 = evaluator.evaluate(snap, cfg, t0 + std::chrono::seconds(10));
    CW_EXPECT_EQ(alerts2.size(), std::size_t{0});

    // After cooldown: re-triggers if still above threshold
    auto alerts3 = evaluator.evaluate(snap, cfg, t0 + std::chrono::seconds(31));
    CW_EXPECT_EQ(alerts3.size(), std::size_t{1});
}

CW_TEST(Win32SensorsAndMonitoringEngineSmoke) {
    MonitoringEngine engine;
    engine.register_provider(std::make_unique<CpuSensorProvider>());
    engine.register_provider(std::make_unique<MemorySensorProvider>());
    engine.register_provider(std::make_unique<StorageSensorProvider>());
    engine.register_provider(std::make_unique<NvmlGpuSensorProvider>());

    engine.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    const auto snap = engine.latest_snapshot();
    engine.stop();

    CW_EXPECT_TRUE(snap.cpu.logical_core_count > 0);
    CW_EXPECT_TRUE(snap.memory.total_bytes.is_available());
    CW_EXPECT_TRUE(*snap.memory.total_bytes.value > 0);
    CW_EXPECT_TRUE(!snap.storage.volumes.empty());
    CW_EXPECT_TRUE(!engine.history(TimeWindow::OneMinute).empty());
}

int main() {
    int passed = 0;
    int failed = 0;
    for (const auto& tc : ::corewatch::test::registry()) {
        try {
            tc.fn();
            std::cout << "[PASS] " << tc.name << "\n";
            ++passed;
        } catch (const std::exception& ex) {
            std::cerr << "[FAIL] " << tc.name << ": " << ex.what() << "\n";
            ++failed;
        }
    }
    std::cout << "Summary: " << passed << " passed, " << failed << " failed.\n";
    return failed == 0 ? 0 : 1;
}
