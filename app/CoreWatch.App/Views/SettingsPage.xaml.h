#pragma once

#include "SettingsPage.g.h"

namespace winrt::CoreWatch::App::implementation {

struct SettingsPage : SettingsPageT<SettingsPage> {
    SettingsPage();

    void OnIntervalChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    void OnThemeChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    void OnToggleChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

private:
    void SaveFromUi();
    bool loading_{true};
};

} // namespace winrt::CoreWatch::App::implementation

namespace winrt::CoreWatch::App::factory_implementation {
struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage> {};
}
