#include "pch.h"
#include "Views/GpuPage.xaml.h"
#if __has_include("GpuPage.g.cpp")
#include "GpuPage.g.cpp"
#endif

#include "App.xaml.h"
#include "corewatch/core/Format.hpp"

#include <algorithm>
#include <format>
#include <vector>

namespace winrt::CoreWatch::App::implementation {

using namespace corewatch::core;
using namespace winrt::Microsoft::UI::Xaml::Controls;

GpuPage::GpuPage() {
    InitializeComponent();

    util_chart_ = std::make_unique<corewatch::ui::LiveChartControl>(
        L"GPU Core Utilization",
        winrt::Windows::UI::ColorHelper::FromArgb(255, 16, 137, 62),
        0.0, 100.0, L"%");
    temp_chart_ = std::make_unique<corewatch::ui::LiveChartControl>(
        L"GPU Temperature",
        winrt::Windows::UI::ColorHelper::FromArgb(255, 209, 52, 56),
        20.0, 100.0, L"°C");

    ChartsHost().Children().Append(util_chart_->Element());
    ChartsHost().Children().Append(temp_chart_->Element());

    Refresh();
}

void GpuPage::TimeWindowCombo_SelectionChanged(
    winrt::Windows::Foundation::IInspectable const&,
    SelectionChangedEventArgs const&) {
    if (!TimeWindowCombo()) {
        return;
    }
    if (auto item = TimeWindowCombo().SelectedItem().try_as<ComboBoxItem>()) {
        if (auto tag = item.Tag().try_as<winrt::hstring>()) {
            if (*tag == L"300") window_ = TimeWindow::FiveMinutes;
            else if (*tag == L"1800") window_ = TimeWindow::ThirtyMinutes;
            else window_ = TimeWindow::OneMinute;
            Refresh();
        }
    }
}

void GpuPage::Refresh() {
    if (!util_chart_ || !temp_chart_) {
        return;
    }
    auto& engine = App::CurrentApp().Engine();
    const auto snap = engine.latest_snapshot();

    if (snap.gpus.empty()) {
        NvmlInfoBar().IsOpen(true);
        GpuModelSubtitle().Text(L"No NVIDIA NVML GPU detected");
        UtilText().Text(L"Unavailable");
        ClocksText().Text(L"Clocks: Unavailable");
        TempText().Text(L"Unavailable");
        FanText().Text(L"Fan Speed: Unavailable");
        PowerText().Text(L"Unavailable");
        PowerLimitText().Text(L"Limit: Unavailable");
        VramText().Text(L"Unavailable");
        VramBar().Value(0.0);
        util_chart_->SetEmpty(L"Unavailable");
        temp_chart_->SetEmpty(L"Unavailable");
        return;
    }

    NvmlInfoBar().IsOpen(false);
    const auto& gpu = snap.gpus[0];

    GpuModelSubtitle().Text(utf8_to_wide(std::format(
        "{}  |  Driver {}", gpu.name, gpu.driver_version.empty() ? "Unknown" : gpu.driver_version)));
    UtilText().Text(utf8_to_wide(format_percent(gpu.utilization_percent)));
    ClocksText().Text(utf8_to_wide(std::format(
        "Core: {}  |  Mem: {}",
        format_clock_mhz(gpu.graphics_clock_mhz),
        format_clock_mhz(gpu.memory_clock_mhz))));

    TempText().Text(utf8_to_wide(format_temperature_c(gpu.temperature_celsius)));
    FanText().Text(utf8_to_wide(std::format("Fan Speed: {}", format_percent(gpu.fan_speed_percent))));
    PowerText().Text(utf8_to_wide(format_watts(gpu.power_watts)));
    PowerLimitText().Text(utf8_to_wide(std::format("Limit: {}", format_watts(gpu.power_limit_watts))));

    if (gpu.vram_used_bytes.is_available() && gpu.vram_total_bytes.is_available() && *gpu.vram_total_bytes.value > 0) {
        const double pct = std::clamp(
            (static_cast<double>(*gpu.vram_used_bytes.value) / static_cast<double>(*gpu.vram_total_bytes.value)) * 100.0,
            0.0, 100.0);
        VramBar().Value(pct);
        VramText().Text(utf8_to_wide(std::format(
            "{} / {} ({:.1f}%)",
            format_bytes(*gpu.vram_used_bytes.value),
            format_bytes(*gpu.vram_total_bytes.value),
            pct)));
    } else {
        VramBar().Value(0.0);
        VramText().Text(L"Unavailable");
    }

    const auto samples = engine.history(window_);
    std::vector<double> util_vals;
    std::vector<double> temp_vals;
    for (const auto& s : samples) {
        if (s.gpu_percent.has_value()) util_vals.push_back(*s.gpu_percent);
        if (s.gpu_temp_celsius.has_value()) temp_vals.push_back(*s.gpu_temp_celsius);
    }

    if (!util_vals.empty()) {
        util_chart_->SetSeries(util_vals, utf8_to_wide(format_percent(gpu.utilization_percent)));
    } else {
        util_chart_->SetEmpty(L"Unavailable");
    }

    if (!temp_vals.empty()) {
        temp_chart_->SetSeries(temp_vals, utf8_to_wide(format_temperature_c(gpu.temperature_celsius)));
    } else {
        temp_chart_->SetEmpty(L"Unavailable");
    }
}

} // namespace winrt::CoreWatch::App::implementation
