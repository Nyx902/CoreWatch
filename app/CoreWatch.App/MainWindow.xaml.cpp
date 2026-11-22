#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "App.xaml.h"
#include "Views/AlertsPage.xaml.h"
#include "Views/CpuPage.xaml.h"
#include "Views/DashboardPage.xaml.h"
#include "Views/GpuPage.xaml.h"
#include "Views/MemoryStoragePage.xaml.h"
#include "Views/SettingsPage.xaml.h"
#include "corewatch/core/Format.hpp"
#include "resource.h"

namespace winrt::CoreWatch::App::implementation {

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media::Animation;

MainWindow::MainWindow() {
    InitializeComponent();
    dispatcher_ = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();

    Title(L"CoreWatch");
    ExtendsContentIntoTitleBar(true);
    SetTitleBar(AppTitleBar());
    SystemBackdrop(winrt::Microsoft::UI::Xaml::Media::MicaBackdrop());

    // Retrieve native HWND for icon, sizing, and tray show/hide
    if (auto native = this->try_as<IWindowNative>()) {
        native->get_WindowHandle(&hwnd_);
    }

    if (hwnd_) {
        HICON hicon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_COREWATCH_ICON));
        if (hicon) {
            SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hicon));
            SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hicon));
        }

        auto window_id = winrt::Microsoft::UI::GetWindowIdFromWindow(hwnd_);
        auto app_window = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(window_id);
        if (app_window) {
            app_window.Resize({1180, 780});
            app_window.Closing([this](auto const&, winrt::Microsoft::UI::Windowing::AppWindowClosingEventArgs const& args) {
                if (!exit_requested_ && App::CurrentApp().Config().minimize_to_tray) {
                    args.Cancel(true);
                    if (hwnd_) {
                        ShowWindow(hwnd_, SW_HIDE);
                    }
                } else {
                    tray_.shutdown();
                    App::CurrentApp().Engine().stop();
                }
            });
        }
    }

    // Initialize system tray
    tray_.initialize(GetModuleHandleW(nullptr), corewatch::tray::TrayIconManager::Callbacks{
        .on_open = [this]() {
            if (dispatcher_) {
                dispatcher_.TryEnqueue([this]() {
                    if (hwnd_) {
                        ShowWindow(hwnd_, SW_SHOW);
                        ShowWindow(hwnd_, SW_RESTORE);
                        SetForegroundWindow(hwnd_);
                    }
                });
            }
        },
        .on_toggle_pause = [this]() {
            auto& engine = App::CurrentApp().Engine();
            engine.set_paused(!engine.is_paused());
            if (dispatcher_) {
                dispatcher_.TryEnqueue([this]() {
                    const bool paused = App::CurrentApp().Engine().is_paused();
                    StatusBadge().Text(paused ? L"Paused" : L"Live");
                });
            }
        },
        .is_paused = []() {
            return App::CurrentApp().Engine().is_paused();
        },
        .on_exit = [this]() {
            if (dispatcher_) {
                dispatcher_.TryEnqueue([]() {
                    App::CurrentApp().RequestExit();
                });
            }
        }
    });

    // Subscribe to MonitoringEngine updates
    auto& engine = App::CurrentApp().Engine();
    engine.set_snapshot_callback([this](const corewatch::core::SystemSnapshot&) {
        if (dispatcher_) {
            dispatcher_.TryEnqueue([this]() {
                RefreshActivePage();
            });
        }
    });
    engine.set_alert_callback([this](const corewatch::engine::AlertEvent& ev) {
        tray_.show_balloon(
            corewatch::core::utf8_to_wide(ev.title),
            corewatch::core::utf8_to_wide(ev.message));
    });

    // Select Dashboard initially
    if (NavView().MenuItems().Size() > 0) {
        NavView().SelectedItem(NavView().MenuItems().GetAt(0));
    }
}

MainWindow::~MainWindow() {
    tray_.shutdown();
}

void MainWindow::PrepareForExit() {
    exit_requested_ = true;
    tray_.shutdown();
}

void MainWindow::NavView_SelectionChanged(
    NavigationView const&,
    NavigationViewSelectionChangedEventArgs const& args) {
    if (args.IsSettingsSelected()) {
        ContentFrame().Navigate(
            winrt::xaml_typename<CoreWatch::App::SettingsPage>(),
            nullptr,
            EntranceNavigationTransitionInfo());
        return;
    }

    if (auto item = args.SelectedItem().try_as<NavigationViewItem>()) {
        if (auto tag = item.Tag().try_as<winrt::hstring>()) {
            NavigateToTag(*tag);
        }
    }
}

void MainWindow::NavigateToTag(winrt::hstring const& tag) {
    EntranceNavigationTransitionInfo transition;
    if (tag == L"dashboard") {
        ContentFrame().Navigate(winrt::xaml_typename<CoreWatch::App::DashboardPage>(), nullptr, transition);
    } else if (tag == L"cpu") {
        ContentFrame().Navigate(winrt::xaml_typename<CoreWatch::App::CpuPage>(), nullptr, transition);
    } else if (tag == L"gpu") {
        ContentFrame().Navigate(winrt::xaml_typename<CoreWatch::App::GpuPage>(), nullptr, transition);
    } else if (tag == L"mem_storage") {
        ContentFrame().Navigate(winrt::xaml_typename<CoreWatch::App::MemoryStoragePage>(), nullptr, transition);
    } else if (tag == L"alerts") {
        ContentFrame().Navigate(winrt::xaml_typename<CoreWatch::App::AlertsPage>(), nullptr, transition);
    }
}

void MainWindow::RefreshActivePage() {
    auto content = ContentFrame().Content();
    if (!content) {
        return;
    }
    if (auto p = content.try_as<CoreWatch::App::DashboardPage>()) {
        winrt::get_self<DashboardPage>(p)->Refresh();
    } else if (auto p2 = content.try_as<CoreWatch::App::CpuPage>()) {
        winrt::get_self<CpuPage>(p2)->Refresh();
    } else if (auto p3 = content.try_as<CoreWatch::App::GpuPage>()) {
        winrt::get_self<GpuPage>(p3)->Refresh();
    } else if (auto p4 = content.try_as<CoreWatch::App::MemoryStoragePage>()) {
        winrt::get_self<MemoryStoragePage>(p4)->Refresh();
    } else if (auto p5 = content.try_as<CoreWatch::App::AlertsPage>()) {
        winrt::get_self<AlertsPage>(p5)->Refresh();
    }
}

} // namespace winrt::CoreWatch::App::implementation
