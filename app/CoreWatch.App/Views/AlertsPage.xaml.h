#pragma once

#include "AlertsPage.g.h"

namespace winrt::CoreWatch::App::implementation {

struct AlertsPage : AlertsPageT<AlertsPage> {
    AlertsPage();

    void OnAlertSettingChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void OnNumberChanged(
        winrt::Microsoft::UI::Xaml::Controls::NumberBox const& sender,
        winrt::Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs const& args);

    void Refresh();

private:
    void SaveFromUi();
    bool loading_{true};
};

} // namespace winrt::CoreWatch::App::implementation

namespace winrt::CoreWatch::App::factory_implementation {
struct AlertsPage : AlertsPageT<AlertsPage, implementation::AlertsPage> {};
}
