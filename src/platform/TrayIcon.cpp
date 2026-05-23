#include "platform/TrayIcon.h"
#include "resource.h"

#include <shellapi.h>

#include <algorithm>

namespace wpcc
{
    TrayIcon::~TrayIcon()
    {
        Destroy();
    }

    bool TrayIcon::Create(HWND hwnd, HINSTANCE instance, UINT iconResId, std::wstring_view tooltip)
    {
        if (m_visible)
        {
            Destroy();
        }

        m_hwnd = hwnd;

        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = m_hwnd;
        nid.uID = m_iconId;
        nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        nid.uCallbackMessage = WM_TRAY_NOTIFY;
        nid.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(iconResId));

        if (nid.hIcon == nullptr)
        {
            return false;
        }

        if (!tooltip.empty())
        {
            const size_t copyLen = (std::min)(tooltip.size(), static_cast<size_t>(127));
            wcsncpy_s(nid.szTip, tooltip.data(), copyLen);
            nid.szTip[copyLen] = L'\0';
        }

        if (!Shell_NotifyIconW(NIM_ADD, &nid))
        {
            return false;
        }

        m_visible = true;
        return true;
    }

    void TrayIcon::Destroy()
    {
        if (!m_visible || m_hwnd == nullptr)
        {
            return;
        }

        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = m_hwnd;
        nid.uID = m_iconId;

        Shell_NotifyIconW(NIM_DELETE, &nid);

        m_visible = false;
        m_hwnd = nullptr;
    }
}
