#pragma once

#include "GpuPage.g.h"
#include "Controls/LiveChartControl.h"
#include "corewatch/core/Types.hpp"

#include <memory>

namespace winrt::CoreWatch::App::implementation {

struct GpuPage : GpuPageT<GpuPage> {
    GpuPage();

    void TimeWindowCombo_SelectionChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    void Refresh();

private:
    corewatch::core::TimeWindow window_{corewatch::core::TimeWindow::OneMinute};
    std::unique_ptr<corewatch::ui::LiveChartControl> util_chart_;
    std::unique_ptr<corewatch::ui::LiveChartControl> temp_chart_;
};

} // namespace winrt::CoreWatch::App::implementation

namespace winrt::CoreWatch::App::factory_implementation {
struct GpuPage : GpuPageT<GpuPage, implementation::GpuPage> {};
}
