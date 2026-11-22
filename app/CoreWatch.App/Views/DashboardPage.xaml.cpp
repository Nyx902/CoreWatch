#include "pch.h"
#include "Views/DashboardPage.xaml.h"
#if __has_include("DashboardPage.g.cpp")
#include "DashboardPage.g.cpp"
#endif

#include "App.xaml.h"
#include "corewatch/core/Format.hpp"

#include <format>
#include <vector>

namespace winrt::CoreWatch::App::implementation {

using namespace corewatch::core;
using namespace winrt::Microsoft::UI::Xaml::Controls;

DashboardPage::DashboardPage() {
    InitializeComponent();

    cpu_chart_ = std::make_unique<corewatch::ui::LiveChartControl>(
        L"CPU Utilization",
        winrt::Windows::UI::ColorHelper::FromArgb(255, 0, 120, 212),
        0.0, 100.0, L"%");
    gpu_chart_ = std::make_unique<corewatch::ui::LiveChartControl>(
        L"GPU Utilization",
        winrt::Windows::UI::ColorHelper::FromArgb(255, 16, 137, 62),
        0.0, 100.0, L"%");
    mem_chart_ = std::make_unique<corewatch::ui::LiveChartControl>(
        L"Memory Usage",
        winrt::Windows::UI::ColorHelper::FromArgb(255, 136, 23, 152),
        0.0, 100.0, L"%");

    ChartsHost().Children().Append(cpu_chart_->Element());
    ChartsHost().Children().Append(gpu_chart_->Element());
    ChartsHost().Children().Append(mem_chart_->Element());

    Refresh();
}

void DashboardPage::TimeWindowCombo_SelectionChanged(
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

void DashboardPage::Refresh() {
    if (!cpu_chart_ || !gpu_chart_ || !mem_chart_) {
        return;
    }
    auto& engine = App::CurrentApp().Engine();
    const auto snap = engine.latest_snapshot();

    // CPU card
    CpuHeroText().Text(utf8_to_wide(format_percent(snap.cpu.total_utilization_percent)));
    CpuNameText().Text(utf8_to_wide(snap.cpu.name));
    CpuDetailText().Text(utf8_to_wide(std::format(
        "Clock: {}  |  {}C / {}T",
        format_clock_mhz(snap.cpu.current_clock_mhz),
        snap.cpu.physical_core_count,
        snap.cpu.logical_core_count)));

    // GPU card
    if (!snap.gpus.empty()) {
        const auto& gpu = snap.gpus[0];
        GpuHeroText().Text(utf8_to_wide(format_percent(gpu.utilization_percent)));
        GpuNameText().Text(utf8_to_wide(gpu.name));
        GpuDetailText().Text(utf8_to_wide(std::format(
            "Temp: {}  |  VRAM: {} / {}",
            format_temperature_c(gpu.temperature_celsius),
            format_bytes_sensor(gpu.vram_used_bytes),
            format_bytes_sensor(gpu.vram_total_bytes))));
    } else {
        GpuHeroText().Text(L"Unavailable");
        GpuNameText().Text(L"NVIDIA NVML not active");
        GpuDetailText().Text(L"Temp: Unavailable");
    }

    // Memory card
    MemHeroText().Text(utf8_to_wide(format_percent(snap.memory.usage_percent)));
    MemUsedText().Text(utf8_to_wide(std::format(
        "Used: {} / {}",
        format_bytes_sensor(snap.memory.used_bytes),
        format_bytes_sensor(snap.memory.total_bytes))));
    MemAvailText().Text(utf8_to_wide(std::format(
        "Available: {}", format_bytes_sensor(snap.memory.available_bytes))));

    // Storage card
    if (!snap.storage.volumes.empty()) {
        const auto& vol = snap.storage.volumes[0];
        DiskHeroText().Text(utf8_to_wide(format_percent(vol.usage_percent)));
        DiskVolumeText().Text(utf8_to_wide(std::format(
            "{}  {} / {}",
            vol.mount_point,
            format_bytes_sensor(vol.used_bytes),
            format_bytes_sensor(vol.total_bytes))));
    } else {
        DiskHeroText().Text(L"Unavailable");
        DiskVolumeText().Text(L"No fixed volumes");
    }
    const std::string r_str = snap.storage.read_bytes_per_sec.is_available()
        ? format_bytes_per_sec(*snap.storage.read_bytes_per_sec.value) : "Unavailable";
    const std::string w_str = snap.storage.write_bytes_per_sec.is_available()
        ? format_bytes_per_sec(*snap.storage.write_bytes_per_sec.value) : "Unavailable";
    DiskThroughputText().Text(utf8_to_wide(std::format("R: {}  |  W: {}", r_str, w_str)));

    // Charts
    const auto samples = engine.history(window_);
    std::vector<double> cpu_vals;
    std::vector<double> gpu_vals;
    std::vector<double> mem_vals;
    cpu_vals.reserve(samples.size());
    mem_vals.reserve(samples.size());

    for (const auto& s : samples) {
        cpu_vals.push_back(s.cpu_percent);
        mem_vals.push_back(s.memory_percent);
        if (s.gpu_percent.has_value()) {
            gpu_vals.push_back(*s.gpu_percent);
        }
    }

    cpu_chart_->SetSeries(cpu_vals, utf8_to_wide(format_percent(snap.cpu.total_utilization_percent)));
    mem_chart_->SetSeries(mem_vals, utf8_to_wide(format_percent(snap.memory.usage_percent)));

    if (!snap.gpus.empty() && !gpu_vals.empty()) {
        gpu_chart_->SetSeries(gpu_vals, utf8_to_wide(format_percent(snap.gpus[0].utilization_percent)));
    } else {
        gpu_chart_->SetEmpty(L"Unavailable");
    }
}

} // namespace winrt::CoreWatch::App::implementation
