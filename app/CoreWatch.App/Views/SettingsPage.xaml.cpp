#include "pch.h"
#include "Views/SettingsPage.xaml.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif

#include "App.xaml.h"

namespace winrt::CoreWatch::App::implementation {

using namespace corewatch::config;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

SettingsPage::SettingsPage() {
    InitializeComponent();

    const auto cfg = App::CurrentApp().Config();

    // Select interval
    for (uint32_t i = 0; i < IntervalCombo().Items().Size(); ++i) {
        if (auto item = IntervalCombo().Items().GetAt(i).try_as<ComboBoxItem>()) {
            if (auto tag = item.Tag().try_as<winrt::hstring>()) {
                if (std::stoul(std::wstring(*tag)) == cfg.polling_interval_ms) {
                    IntervalCombo().SelectedIndex(static_cast<int32_t>(i));
                    break;
                }
            }
        }
    }
    if (IntervalCombo().SelectedIndex() < 0) {
        IntervalCombo().SelectedIndex(2); // 1000ms
    }

    // Select theme
    switch (cfg.theme) {
        case ThemePreference::Light: ThemeCombo().SelectedIndex(1); break;
        case ThemePreference::Dark: ThemeCombo().SelectedIndex(2); break;
        case ThemePreference::System:
        default: ThemeCombo().SelectedIndex(0); break;
    }

    StartupToggle().IsOn(cfg.launch_at_startup);
    TrayToggle().IsOn(cfg.minimize_to_tray);
    ConfigPathText().Text(App::CurrentApp().ConfigStore().path().wstring());

    loading_ = false;
}

void SettingsPage::OnIntervalChanged(winrt::Windows::Foundation::IInspectable const&, SelectionChangedEventArgs const&) {
    if (!loading_) SaveFromUi();
}

void SettingsPage::OnThemeChanged(winrt::Windows::Foundation::IInspectable const&, SelectionChangedEventArgs const&) {
    if (!loading_) SaveFromUi();
}

void SettingsPage::OnToggleChanged(winrt::Windows::Foundation::IInspectable const&, RoutedEventArgs const&) {
    if (!loading_) SaveFromUi();
}

void SettingsPage::SaveFromUi() {
    auto cfg = App::CurrentApp().Config();

    if (auto item = IntervalCombo().SelectedItem().try_as<ComboBoxItem>()) {
        if (auto tag = item.Tag().try_as<winrt::hstring>()) {
            cfg.polling_interval_ms = static_cast<std::uint32_t>(std::stoul(std::wstring(*tag)));
        }
    }

    if (auto item = ThemeCombo().SelectedItem().try_as<ComboBoxItem>()) {
        if (auto tag = item.Tag().try_as<winrt::hstring>()) {
            if (*tag == L"light") cfg.theme = ThemePreference::Light;
            else if (*tag == L"dark") cfg.theme = ThemePreference::Dark;
            else cfg.theme = ThemePreference::System;
        }
    }

    cfg.launch_at_startup = StartupToggle().IsOn();
    cfg.minimize_to_tray = TrayToggle().IsOn();

    App::CurrentApp().ApplyAndSaveConfig(cfg);
}

} // namespace winrt::CoreWatch::App::implementation
