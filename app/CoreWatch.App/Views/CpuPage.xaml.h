#pragma once

#include "CpuPage.g.h"
#include "Controls/LiveChartControl.h"
#include "corewatch/core/Types.hpp"

#include <memory>
#include <vector>

namespace winrt::CoreWatch::App::implementation {

struct CpuPage : CpuPageT<CpuPage> {
    CpuPage();

    void TimeWindowCombo_SelectionChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    void Refresh();

private:
    struct CoreTile {
        winrt::Microsoft::UI::Xaml::Controls::TextBlock label{nullptr};
        winrt::Microsoft::UI::Xaml::Controls::ProgressBar bar{nullptr};
    };

    void EnsureCoreTiles(std::size_t count);

    corewatch::core::TimeWindow window_{corewatch::core::TimeWindow::OneMinute};
    std::unique_ptr<corewatch::ui::LiveChartControl> cpu_chart_;
    std::vector<CoreTile> core_tiles_;
};

} // namespace winrt::CoreWatch::App::implementation

namespace winrt::CoreWatch::App::factory_implementation {
struct CpuPage : CpuPageT<CpuPage, implementation::CpuPage> {};
}
