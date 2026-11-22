#include "pch.h"
#include "Views/AlertsPage.xaml.h"
#if __has_include("AlertsPage.g.cpp")
#include "AlertsPage.g.cpp"
#endif

#include "App.xaml.h"
#include "corewatch/core/Format.hpp"

#include <cmath>

namespace winrt::CoreWatch::App::implementation {

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

AlertsPage::AlertsPage() {
    InitializeComponent();

    const auto cfg = App::CurrentApp().Config();
    AlertsEnabledToggle().IsOn(cfg.alerts_enabled);
    GpuThresholdBox().Value(cfg.gpu_temp_threshold_c);
    CpuThresholdBox().Value(cfg.cpu_temp_threshold_c);
    CooldownBox().Value(static_cast<double>(cfg.alert_cooldown_seconds));
    loading_ = false;

    Refresh();
}

void AlertsPage::OnAlertSettingChanged(winrt::Windows::Foundation::IInspectable const&, RoutedEventArgs const&) {
    if (!loading_) {
        SaveFromUi();
    }
}

void AlertsPage::OnNumberChanged(NumberBox const&, NumberBoxValueChangedEventArgs const&) {
    if (!loading_) {
        SaveFromUi();
    }
}

void AlertsPage::SaveFromUi() {
    auto cfg = App::CurrentApp().Config();
    cfg.alerts_enabled = AlertsEnabledToggle().IsOn();
    if (!std::isnan(GpuThresholdBox().Value())) {
        cfg.gpu_temp_threshold_c = GpuThresholdBox().Value();
    }
    if (!std::isnan(CpuThresholdBox().Value())) {
        cfg.cpu_temp_threshold_c = CpuThresholdBox().Value();
    }
    if (!std::isnan(CooldownBox().Value())) {
        cfg.alert_cooldown_seconds = static_cast<std::uint32_t>(CooldownBox().Value());
    }
    App::CurrentApp().ApplyAndSaveConfig(cfg);
}

void AlertsPage::Refresh() {
    const auto alerts = App::CurrentApp().Engine().recent_alerts();
    AlertsListPanel().Children().Clear();
    EmptyAlertsText().Visibility(alerts.empty() ? Visibility::Visible : Visibility::Collapsed);

    for (const auto& ev : alerts) {
        Border row;
        row.Padding(ThicknessHelper::FromUniformLength(10));
        row.CornerRadius(CornerRadiusHelper::FromUniformRadius(6));
        row.Background(Application::Current().Resources().Lookup(winrt::box_value(L"SubtleFillColorSecondaryBrush")).as<winrt::Microsoft::UI::Xaml::Media::Brush>());

        StackPanel sp;
        sp.Spacing(2);
        TextBlock title;
        title.Text(corewatch::core::utf8_to_wide(ev.title));
        title.FontWeight(winrt::Microsoft::UI::Text::FontWeights::SemiBold());
        TextBlock msg;
        msg.Text(corewatch::core::utf8_to_wide(ev.message));
        msg.Opacity(0.8);

        sp.Children().Append(title);
        sp.Children().Append(msg);
        row.Child(sp);
        AlertsListPanel().Children().Append(row);
    }
}

} // namespace winrt::CoreWatch::App::implementation
