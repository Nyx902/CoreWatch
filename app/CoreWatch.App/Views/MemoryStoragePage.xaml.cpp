#include "pch.h"
#include "Views/MemoryStoragePage.xaml.h"
#if __has_include("MemoryStoragePage.g.cpp")
#include "MemoryStoragePage.g.cpp"
#endif

#include "App.xaml.h"
#include "corewatch/core/Format.hpp"

#include <format>

namespace winrt::CoreWatch::App::implementation {

using namespace corewatch::core;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

MemoryStoragePage::MemoryStoragePage() {
    InitializeComponent();
    Refresh();
}

void MemoryStoragePage::Refresh() {
    const auto snap = App::CurrentApp().Engine().latest_snapshot();
    const auto& mem = snap.memory;

    MemPercentText().Text(utf8_to_wide(format_percent(mem.usage_percent)));
    MemBar().Value(mem.usage_percent.value.value_or(0.0));
    MemTotalText().Text(utf8_to_wide(format_bytes_sensor(mem.total_bytes)));
    MemUsedText().Text(utf8_to_wide(format_bytes_sensor(mem.used_bytes)));
    MemAvailText().Text(utf8_to_wide(format_bytes_sensor(mem.available_bytes)));
    MemCommitText().Text(utf8_to_wide(std::format(
        "{} / {}",
        format_bytes_sensor(mem.commit_total_bytes),
        format_bytes_sensor(mem.commit_limit_bytes))));

    const auto& st = snap.storage;
    DiskActiveText().Text(utf8_to_wide(format_percent(st.disk_active_time_percent)));
    DiskReadText().Text(st.read_bytes_per_sec.is_available()
        ? utf8_to_wide(format_bytes_per_sec(*st.read_bytes_per_sec.value))
        : L"Unavailable");
    DiskWriteText().Text(st.write_bytes_per_sec.is_available()
        ? utf8_to_wide(format_bytes_per_sec(*st.write_bytes_per_sec.value))
        : L"Unavailable");

    VolumesPanel().Children().Clear();
    for (const auto& vol : st.volumes) {
        Border card;
        card.Padding(ThicknessHelper::FromUniformLength(12));
        card.CornerRadius(CornerRadiusHelper::FromUniformRadius(6));
        card.Background(Application::Current().Resources().Lookup(winrt::box_value(L"SubtleFillColorSecondaryBrush")).as<winrt::Microsoft::UI::Xaml::Media::Brush>());

        StackPanel sp;
        sp.Spacing(6);

        Grid header;
        TextBlock name_tb;
        const std::string title = vol.volume_label.empty()
            ? std::format("{} ({})", vol.mount_point, vol.file_system)
            : std::format("{} {} ({})", vol.mount_point, vol.volume_label, vol.file_system);
        name_tb.Text(utf8_to_wide(title));
        name_tb.FontWeight(winrt::Microsoft::UI::Text::FontWeights::SemiBold());

        TextBlock cap_tb;
        cap_tb.Text(utf8_to_wide(std::format(
            "{} free of {}  ({})",
            format_bytes_sensor(vol.free_bytes),
            format_bytes_sensor(vol.total_bytes),
            format_percent(vol.usage_percent))));
        cap_tb.HorizontalAlignment(HorizontalAlignment::Right);
        cap_tb.Opacity(0.75);

        header.Children().Append(name_tb);
        header.Children().Append(cap_tb);

        ProgressBar pb;
        pb.Minimum(0.0);
        pb.Maximum(100.0);
        pb.Value(vol.usage_percent.value.value_or(0.0));
        pb.Height(6.0);
        pb.CornerRadius(CornerRadiusHelper::FromUniformRadius(3));

        sp.Children().Append(header);
        sp.Children().Append(pb);
        card.Child(sp);
        VolumesPanel().Children().Append(card);
    }
}

} // namespace winrt::CoreWatch::App::implementation
