#pragma once

#include <Windows.h>

#include <string_view>

namespace wpcc
{
    constexpr UINT WM_TRAY_NOTIFY = WM_APP + 1;

    class TrayIcon
    {
    public:
        TrayIcon() = default;
        ~TrayIcon();

        TrayIcon(const TrayIcon&) = delete;
        TrayIcon& operator=(const TrayIcon&) = delete;

        bool Create(HWND hwnd, HINSTANCE instance, UINT iconResId, std::wstring_view tooltip);
        void Destroy();

        HWND GetTargetWindow() const { return m_hwnd; }

    private:
        HWND m_hwnd = nullptr;
        bool m_visible = false;

        static constexpr UINT m_iconId = 1;
    };
}
