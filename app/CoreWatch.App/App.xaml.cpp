#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"

#include "corewatch/sensors/CpuSensorProvider.hpp"
#include "corewatch/sensors/MemorySensorProvider.hpp"
#include "corewatch/sensors/NvmlGpuSensorProvider.hpp"
#include "corewatch/sensors/StorageSensorProvider.hpp"

#include <fstream>

namespace winrt::CoreWatch::App::implementation {

App* App::instance_ = nullptr;

App::App() {
    instance_ = this;
    InitializeComponent();
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException([](winrt::Windows::Foundation::IInspectable const&,
                          winrt::Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e) {
        if (IsDebuggerPresent()) {
            auto errorMessage = e.Message();
            __debugbreak();
        }
    });
#endif
}

App::~App() {
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

App& App::CurrentApp() {
    return *instance_;
}

void App::OnLaunched(winrt::Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) {
    try {
        auto cfg = config_store_.load();
        engine_ = std::make_unique<corewatch::engine::MonitoringEngine>(cfg);
        engine_->register_provider(std::make_unique<corewatch::sensors::CpuSensorProvider>());
        engine_->register_provider(std::make_unique<corewatch::sensors::MemorySensorProvider>());
        engine_->register_provider(std::make_unique<corewatch::sensors::StorageSensorProvider>());
        engine_->register_provider(std::make_unique<corewatch::sensors::NvmlGpuSensorProvider>());
        engine_->start();

        window_ = winrt::make<MainWindow>();
        ApplyTheme(cfg.theme);
        window_.Activate();
    } catch (winrt::hresult_error const& ex) {
        const auto log_path = config_store_.path().parent_path() / L"startup_error.log";
        std::filesystem::create_directories(log_path.parent_path());
        std::wofstream out(log_path, std::ios::app);
        out << L"WinRT error: 0x" << std::hex << ex.code().value << L" - " << ex.message().c_str() << L"\n";
    } catch (std::exception const& ex) {
        const auto log_path = config_store_.path().parent_path() / L"startup_error.log";
        std::filesystem::create_directories(log_path.parent_path());
        std::ofstream out(log_path, std::ios::app);
        out << "std::exception: " << ex.what() << "\n";
    }
}

corewatch::config::AppConfig App::Config() const {
    return engine_ ? engine_->config() : config_store_.load();
}

void App::ApplyAndSaveConfig(const corewatch::config::AppConfig& cfg) {
    auto sanitized = cfg;
    sanitized.sanitize();
    config_store_.save(sanitized);
    if (engine_) {
        engine_->update_config(sanitized);
    }
    ApplyTheme(sanitized.theme);

    wchar_t exe_path[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, exe_path, MAX_PATH) > 0) {
        corewatch::config::ConfigStore::set_windows_startup(sanitized.launch_at_startup, exe_path);
    }
}

void App::ApplyTheme(corewatch::config::ThemePreference theme) {
    if (!window_) {
        return;
    }
    auto content = window_.Content().try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();
    if (!content) {
        return;
    }
    switch (theme) {
        case corewatch::config::ThemePreference::Light:
            content.RequestedTheme(winrt::Microsoft::UI::Xaml::ElementTheme::Light);
            break;
        case corewatch::config::ThemePreference::Dark:
            content.RequestedTheme(winrt::Microsoft::UI::Xaml::ElementTheme::Dark);
            break;
        case corewatch::config::ThemePreference::System:
        default:
            content.RequestedTheme(winrt::Microsoft::UI::Xaml::ElementTheme::Default);
            break;
    }
}

void App::RequestExit() {
    if (engine_) {
        engine_->stop();
    }
    if (window_) {
        if (auto main_win = winrt::get_self<MainWindow>(window_.as<CoreWatch::App::MainWindow>())) {
            main_win->PrepareForExit();
        }
        window_.Close();
        window_ = nullptr;
    }
    Exit();
}

} // namespace winrt::CoreWatch::App::implementation

#include "XamlMetaDataProvider.g.cpp"
