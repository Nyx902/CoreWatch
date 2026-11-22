#pragma once

#include "App.xaml.g.h"

#include "corewatch/config/AppConfig.hpp"
#include "corewatch/config/ConfigStore.hpp"
#include "corewatch/engine/MonitoringEngine.hpp"

#include <memory>

namespace winrt::CoreWatch::App::implementation {

struct App : AppT<App> {
    App();
    ~App();

    void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    static App& CurrentApp();

    corewatch::engine::MonitoringEngine& Engine() noexcept { return *engine_; }
    corewatch::config::ConfigStore& ConfigStore() noexcept { return config_store_; }
    corewatch::config::AppConfig Config() const;
    void ApplyAndSaveConfig(const corewatch::config::AppConfig& cfg);
    void ApplyTheme(corewatch::config::ThemePreference theme);
    void RequestExit();

private:
    static App* instance_;
    winrt::Microsoft::UI::Xaml::Window window_{nullptr};
    corewatch::config::ConfigStore config_store_;
    std::unique_ptr<corewatch::engine::MonitoringEngine> engine_;
};

} // namespace winrt::CoreWatch::App::implementation
