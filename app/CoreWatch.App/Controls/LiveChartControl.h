#pragma once

#include <string>
#include <vector>

namespace corewatch::ui {

/// Native WinUI 3 real-time line/area chart rendered with XAML Shapes (Canvas + Polyline + Polygon).
/// Requires no WebView2 or external charting runtime.
class LiveChartControl {
public:
    LiveChartControl(
        std::wstring title,
        winrt::Windows::UI::Color stroke_color,
        double min_value = 0.0,
        double max_value = 100.0,
        std::wstring unit_suffix = L"%");

    [[nodiscard]] winrt::Microsoft::UI::Xaml::UIElement Element() const noexcept { return root_; }

    void SetSeries(const std::vector<double>& values, const std::wstring& current_label);
    void SetEmpty(const std::wstring& message = L"Unavailable");

private:
    void Redraw();

    std::wstring title_;
    winrt::Windows::UI::Color stroke_color_;
    double min_value_{0.0};
    double max_value_{100.0};
    std::wstring unit_suffix_;
    std::vector<double> values_;

    winrt::Microsoft::UI::Xaml::Controls::Border root_{nullptr};
    winrt::Microsoft::UI::Xaml::Controls::TextBlock value_text_{nullptr};
    winrt::Microsoft::UI::Xaml::Controls::TextBlock empty_text_{nullptr};
    winrt::Microsoft::UI::Xaml::Controls::Canvas canvas_{nullptr};
    winrt::Microsoft::UI::Xaml::Shapes::Polyline line_{nullptr};
    winrt::Microsoft::UI::Xaml::Shapes::Polygon area_{nullptr};
    std::vector<winrt::Microsoft::UI::Xaml::Shapes::Line> grid_lines_;
};

} // namespace corewatch::ui
