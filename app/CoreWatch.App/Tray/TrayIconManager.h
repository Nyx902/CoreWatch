#pragma once

#include <functional>
#include <string>

#include <windows.h>

namespace corewatch::tray {

class TrayIconManager {
public:
    struct Callbacks {
        std::function<void()> on_open;
        std::function<void()> on_toggle_pause;
        std::function<bool()> is_paused;
        std::function<void()> on_exit;
    };

    TrayIconManager() = default;
    ~TrayIconManager();

    TrayIconManager(const TrayIconManager&) = delete;
    TrayIconManager& operator=(const TrayIconManager&) = delete;

    bool initialize(HINSTANCE instance, Callbacks callbacks);
    void shutdown();
    void show_balloon(const std::wstring& title, const std::wstring& message);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    void show_context_menu();

    HWND msg_hwnd_{nullptr};
    HICON icon_{nullptr};
    bool added_{false};
    Callbacks callbacks_{};
};

} // namespace corewatch::tray
