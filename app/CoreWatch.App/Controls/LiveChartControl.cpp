#include "pch.h"
#include "Controls/LiveChartControl.h"

#include <algorithm>
#include <format>

namespace corewatch::ui {

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;
using namespace winrt::Microsoft::UI::Xaml::Shapes;

LiveChartControl::LiveChartControl(
    std::wstring title,
    winrt::Windows::UI::Color stroke_color,
    double min_value,
    double max_value,
    std::wstring unit_suffix)
    : title_(std::move(title))
    , stroke_color_(stroke_color)
    , min_value_(min_value)
    , max_value_(max_value)
    , unit_suffix_(std::move(unit_suffix)) {
    root_ = Border();
    root_.CornerRadius(CornerRadiusHelper::FromUniformRadius(8.0));
    root_.BorderThickness(ThicknessHelper::FromUniformLength(1.0));
    root_.Padding(ThicknessHelper::FromUniformLength(16.0));
    if (auto bg = Application::Current().Resources().TryLookup(winrt::box_value(L"CardBackgroundFillColorDefaultBrush"))) {
        root_.Background(bg.try_as<Brush>());
    }
    if (auto border = Application::Current().Resources().TryLookup(winrt::box_value(L"CardStrokeColorDefaultBrush"))) {
        root_.BorderBrush(border.try_as<Brush>());
    }

    auto layout = Grid();
    RowDefinition r0{};
    r0.Height(GridLengthHelper::Auto());
    RowDefinition r1{};
    r1.Height(GridLengthHelper::FromPixels(150.0));
    layout.RowDefinitions().Append(r0);
    layout.RowDefinitions().Append(r1);

    // Header row
    auto header = Grid();
    header.Margin(ThicknessHelper::FromLengths(0, 0, 0, 12));
    auto title_tb = TextBlock();
    title_tb.Text(title_);
    title_tb.FontSize(14.0);
    title_tb.FontWeight(winrt::Microsoft::UI::Text::FontWeights::SemiBold());
    title_tb.VerticalAlignment(VerticalAlignment::Center);

    value_text_ = TextBlock();
    value_text_.Text(L"--");
    value_text_.FontSize(16.0);
    value_text_.FontWeight(winrt::Microsoft::UI::Text::FontWeights::SemiBold());
    value_text_.Foreground(SolidColorBrush(stroke_color_));
    value_text_.HorizontalAlignment(HorizontalAlignment::Right);
    value_text_.VerticalAlignment(VerticalAlignment::Center);

    header.Children().Append(title_tb);
    header.Children().Append(value_text_);
    Grid::SetRow(header, 0);
    layout.Children().Append(header);

    // Chart area
    auto chart_grid = Grid();
    Grid::SetRow(chart_grid, 1);

    ColumnDefinition c0{};
    c0.Width(GridLengthHelper::FromPixels(42.0));
    ColumnDefinition c1{};
    c1.Width(GridLengthHelper::FromValueAndType(1.0, GridUnitType::Star));
    chart_grid.ColumnDefinitions().Append(c0);
    chart_grid.ColumnDefinitions().Append(c1);

    // Y-axis labels
    auto axis_panel = Grid();
    auto top_lbl = TextBlock();
    top_lbl.Text(std::format(L"{:.0f}{}", max_value_, unit_suffix_));
    top_lbl.FontSize(11.0);
    top_lbl.Opacity(0.6);
    top_lbl.VerticalAlignment(VerticalAlignment::Top);

    auto mid_lbl = TextBlock();
    mid_lbl.Text(std::format(L"{:.0f}{}", (min_value_ + max_value_) * 0.5, unit_suffix_));
    mid_lbl.FontSize(11.0);
    mid_lbl.Opacity(0.6);
    mid_lbl.VerticalAlignment(VerticalAlignment::Center);

    auto bot_lbl = TextBlock();
    bot_lbl.Text(std::format(L"{:.0f}{}", min_value_, unit_suffix_));
    bot_lbl.FontSize(11.0);
    bot_lbl.Opacity(0.6);
    bot_lbl.VerticalAlignment(VerticalAlignment::Bottom);

    axis_panel.Children().Append(top_lbl);
    axis_panel.Children().Append(mid_lbl);
    axis_panel.Children().Append(bot_lbl);
    Grid::SetColumn(axis_panel, 0);
    chart_grid.Children().Append(axis_panel);

    canvas_ = Canvas();
    canvas_.Clip(RectangleGeometry());
    Grid::SetColumn(canvas_, 1);

    auto grid_brush = SolidColorBrush(winrt::Windows::UI::ColorHelper::FromArgb(40, 128, 128, 128));
    for (int i = 0; i < 5; ++i) {
        Line gl;
        gl.Stroke(grid_brush);
        gl.StrokeThickness(1.0);
        grid_lines_.push_back(gl);
        canvas_.Children().Append(gl);
    }

    auto fill_color = stroke_color_;
    fill_color.A = 45;
    area_ = winrt::Microsoft::UI::Xaml::Shapes::Polygon();
    area_.Fill(SolidColorBrush(fill_color));
    canvas_.Children().Append(area_);

    line_ = winrt::Microsoft::UI::Xaml::Shapes::Polyline();
    line_.Stroke(SolidColorBrush(stroke_color_));
    line_.StrokeThickness(2.0);
    line_.StrokeLineJoin(PenLineJoin::Round);
    canvas_.Children().Append(line_);

    empty_text_ = TextBlock();
    empty_text_.Text(L"Waiting for samples...");
    empty_text_.Opacity(0.6);
    empty_text_.HorizontalAlignment(HorizontalAlignment::Center);
    empty_text_.VerticalAlignment(VerticalAlignment::Center);
    empty_text_.Visibility(Visibility::Collapsed);
    Grid::SetColumn(empty_text_, 1);

    chart_grid.Children().Append(canvas_);
    chart_grid.Children().Append(empty_text_);
    layout.Children().Append(chart_grid);

    canvas_.SizeChanged([this](auto const&, SizeChangedEventArgs const&) {
        Redraw();
    });

    root_.Child(layout);
}

void LiveChartControl::SetSeries(const std::vector<double>& values, const std::wstring& current_label) {
    values_ = values;
    value_text_.Text(current_label);
    empty_text_.Visibility(values_.empty() ? Visibility::Visible : Visibility::Collapsed);
    Redraw();
}

void LiveChartControl::SetEmpty(const std::wstring& message) {
    values_.clear();
    value_text_.Text(message);
    empty_text_.Text(message);
    empty_text_.Visibility(Visibility::Visible);
    Redraw();
}

void LiveChartControl::Redraw() {
    const double w = canvas_.ActualWidth();
    const double h = canvas_.ActualHeight();
    if (w <= 2.0 || h <= 2.0) {
        return;
    }

    RectangleGeometry clip;
    clip.Rect(winrt::Windows::Foundation::Rect{0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)});
    canvas_.Clip(clip);

    for (std::size_t i = 0; i < grid_lines_.size(); ++i) {
        const double y = (static_cast<double>(i) / 4.0) * (h - 2.0) + 1.0;
        grid_lines_[i].X1(0.0);
        grid_lines_[i].X2(w);
        grid_lines_[i].Y1(y);
        grid_lines_[i].Y2(y);
    }

    PointCollection line_pts;
    PointCollection area_pts;

    if (values_.size() >= 2) {
        const double span = std::max(1.0, max_value_ - min_value_);
        const double step_x = w / static_cast<double>(values_.size() - 1);

        area_pts.Append(winrt::Windows::Foundation::Point{0.0f, static_cast<float>(h)});
        for (std::size_t i = 0; i < values_.size(); ++i) {
            const double norm = std::clamp((values_[i] - min_value_) / span, 0.0, 1.0);
            const float x = static_cast<float>(static_cast<double>(i) * step_x);
            const float y = static_cast<float>((1.0 - norm) * (h - 4.0) + 2.0);
            winrt::Windows::Foundation::Point pt{x, y};
            line_pts.Append(pt);
            area_pts.Append(pt);
        }
        area_pts.Append(winrt::Windows::Foundation::Point{static_cast<float>(w), static_cast<float>(h)});
    } else if (values_.size() == 1) {
        const double span = std::max(1.0, max_value_ - min_value_);
        const double norm = std::clamp((values_[0] - min_value_) / span, 0.0, 1.0);
        const float y = static_cast<float>((1.0 - norm) * (h - 4.0) + 2.0);
        line_pts.Append(winrt::Windows::Foundation::Point{0.0f, y});
        line_pts.Append(winrt::Windows::Foundation::Point{static_cast<float>(w), y});
    }

    line_.Points(line_pts);
    area_.Points(area_pts);
}

} // namespace corewatch::ui
