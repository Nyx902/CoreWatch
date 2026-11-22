#include "pch.h"
#include "Tray/TrayIconManager.h"
#include "resource.h"

#include <shellapi.h>

namespace corewatch::tray {

namespace {
constexpr UINT kTrayCallbackMsg = WM_USER + 42;
constexpr UINT kTrayIconId = 1;
constexpr UINT_PTR kMenuOpenId = 1001;
constexpr UINT_PTR kMenuPauseId = 1002;
constexpr UINT_PTR kMenuExitId = 1003;
constexpr const wchar_t* kTrayWndClass = L"CoreWatchTrayMsgWindow";
} // namespace

TrayIconManager::~TrayIconManager() {
    shutdown();
}

bool TrayIconManager::initialize(HINSTANCE instance, Callbacks callbacks) {
    callbacks_ = std::move(callbacks);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &TrayIconManager::WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = kTrayWndClass;
    RegisterClassExW(&wc);

    msg_hwnd_ = CreateWindowExW(
        0, kTrayWndClass, L"CoreWatch Tray", 0,
        0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, this);
    if (!msg_hwnd_) {
        return false;
    }

    icon_ = LoadIconW(instance, MAKEINTRESOURCEW(IDI_COREWATCH_ICON));
    if (!icon_) {
        icon_ = LoadIconW(nullptr, IDI_APPLICATION);
    }

    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = msg_hwnd_;
    nid.uID = kTrayIconId;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = kTrayCallbackMsg;
    nid.hIcon = icon_;
    wcscpy_s(nid.szTip, L"CoreWatch — Hardware Monitor");

    added_ = Shell_NotifyIconW(NIM_ADD, &nid) == TRUE;
    return added_;
}

void TrayIconManager::shutdown() {
    if (added_ && msg_hwnd_) {
        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(nid);
        nid.hWnd = msg_hwnd_;
        nid.uID = kTrayIconId;
        Shell_NotifyIconW(NIM_DELETE, &nid);
        added_ = false;
    }
    if (msg_hwnd_) {
        DestroyWindow(msg_hwnd_);
        msg_hwnd_ = nullptr;
    }
}

void TrayIconManager::show_balloon(const std::wstring& title, const std::wstring& message) {
    if (!added_ || !msg_hwnd_) {
        return;
    }
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = msg_hwnd_;
    nid.uID = kTrayIconId;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_WARNING;
    wcsncpy_s(nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(nid.szInfo, message.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void TrayIconManager::show_context_menu() {
    POINT pt{};
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    AppendMenuW(menu, MF_STRING, kMenuOpenId, L"Open CoreWatch");
    const bool paused = callbacks_.is_paused ? callbacks_.is_paused() : false;
    AppendMenuW(menu, MF_STRING, kMenuPauseId, paused ? L"Resume Monitoring" : L"Pause Monitoring");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kMenuExitId, L"Exit");

    SetForegroundWindow(msg_hwnd_);
    const UINT cmd = TrackPopupMenu(
        menu,
        TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, msg_hwnd_, nullptr);
    DestroyMenu(menu);

    if (cmd == kMenuOpenId && callbacks_.on_open) {
        callbacks_.on_open();
    } else if (cmd == kMenuPauseId && callbacks_.on_toggle_pause) {
        callbacks_.on_toggle_pause();
    } else if (cmd == kMenuExitId && callbacks_.on_exit) {
        callbacks_.on_exit();
    }
}

LRESULT CALLBACK TrayIconManager::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    auto* self = reinterpret_cast<TrayIconManager*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self && msg == kTrayCallbackMsg) {
        if (lparam == WM_LBUTTONDBLCLK || lparam == WM_LBUTTONUP) {
            if (self->callbacks_.on_open) {
                self->callbacks_.on_open();
            }
        } else if (lparam == WM_RBUTTONUP || lparam == WM_CONTEXTMENU) {
            self->show_context_menu();
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

} // namespace corewatch::tray
