#include "pch.h"
#include "Views/CpuPage.xaml.h"
#if __has_include("CpuPage.g.cpp")
#include "CpuPage.g.cpp"
#endif

#include "App.xaml.h"
#include "corewatch/core/Format.hpp"

#include <format>

namespace winrt::CoreWatch::App::implementation {

using namespace corewatch::core;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

CpuPage::CpuPage() {
    InitializeComponent();

    cpu_chart_ = std::make_unique<corewatch::ui::LiveChartControl>(
        L"Total CPU Utilization",
        winrt::Windows::UI::ColorHelper::FromArgb(255, 0, 120, 212),
        0.0, 100.0, L"%");
    ChartHost().Children().Append(cpu_chart_->Element());

    Refresh();
}

void CpuPage::TimeWindowCombo_SelectionChanged(
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

void CpuPage::EnsureCoreTiles(std::size_t count) {
    if (core_tiles_.size() == count) {
        return;
    }
    LogicalCoresPanel().Children().Clear();
    core_tiles_.clear();
    core_tiles_.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        Border tile;
        tile.Padding(ThicknessHelper::FromUniformLength(10));
        tile.Margin(ThicknessHelper::FromLengths(0, 0, 8, 8));
        tile.CornerRadius(CornerRadiusHelper::FromUniformRadius(6));
        tile.Background(Application::Current().Resources().Lookup(winrt::box_value(L"SubtleFillColorSecondaryBrush")).as<winrt::Microsoft::UI::Xaml::Media::Brush>());

        StackPanel sp;
        sp.Spacing(6);

        TextBlock lbl;
        lbl.Text(std::format(L"Core {}  --", i));
        lbl.FontSize(12);

        ProgressBar pb;
        pb.Minimum(0.0);
        pb.Maximum(100.0);
        pb.Value(0.0);
        pb.Height(6.0);
        pb.CornerRadius(CornerRadiusHelper::FromUniformRadius(3));

        sp.Children().Append(lbl);
        sp.Children().Append(pb);
        tile.Child(sp);

        LogicalCoresPanel().Children().Append(tile);
        core_tiles_.push_back(CoreTile{lbl, pb});
    }
}

void CpuPage::Refresh() {
    if (!cpu_chart_) {
        return;
    }
    auto& engine = App::CurrentApp().Engine();
    const auto snap = engine.latest_snapshot();
    const auto& cpu = snap.cpu;

    CpuModelSubtitle().Text(utf8_to_wide(cpu.name));
    UtilText().Text(utf8_to_wide(format_percent(cpu.total_utilization_percent)));
    CoresText().Text(utf8_to_wide(std::format("{} Physical / {} Logical", cpu.physical_core_count, cpu.logical_core_count)));
    ClockText().Text(utf8_to_wide(format_clock_mhz(cpu.current_clock_mhz)));
    MaxClockText().Text(utf8_to_wide(std::format("Max: {}", format_clock_mhz(cpu.max_clock_mhz))));
    TempText().Text(utf8_to_wide(format_temperature_c(cpu.temperature_celsius)));
    PowerText().Text(utf8_to_wide(format_watts(cpu.power_watts)));

    const auto samples = engine.history(window_);
    std::vector<double> vals;
    vals.reserve(samples.size());
    for (const auto& s : samples) {
        vals.push_back(s.cpu_percent);
    }
    cpu_chart_->SetSeries(vals, utf8_to_wide(format_percent(cpu.total_utilization_percent)));

    EnsureCoreTiles(cpu.logical_cores.size());
    for (std::size_t i = 0; i < cpu.logical_cores.size(); ++i) {
        const auto& c = cpu.logical_cores[i];
        const double pct = c.utilization_percent.value.value_or(0.0);
        core_tiles_[i].label.Text(utf8_to_wide(std::format("CPU {}  {}", c.core_index, format_percent(c.utilization_percent))));
        core_tiles_[i].bar.Value(pct);
    }
}

} // namespace winrt::CoreWatch::App::implementation
