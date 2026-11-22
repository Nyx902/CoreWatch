#pragma once

#include "MainWindow.g.h"
#include "Tray/TrayIconManager.h"

namespace winrt::CoreWatch::App::implementation {

struct MainWindow : MainWindowT<MainWindow> {
    MainWindow();
    ~MainWindow();

    void NavView_SelectionChanged(
        winrt::Microsoft::UI::Xaml::Controls::NavigationView const& sender,
        winrt::Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args);

    void PrepareForExit();

private:
    void NavigateToTag(winrt::hstring const& tag);
    void RefreshActivePage();

    HWND hwnd_{nullptr};
    bool exit_requested_{false};
    corewatch::tray::TrayIconManager tray_;
    winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher_{nullptr};
};

} // namespace winrt::CoreWatch::App::implementation

namespace winrt::CoreWatch::App::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}
