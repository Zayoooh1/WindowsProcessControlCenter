#include "app/Application.h"
#include "resource.h"

#include <Windows.h>
#include <objbase.h>

namespace
{
    constexpr UINT TRAY_CMD_OPEN = 1;
    constexpr UINT TRAY_CMD_REFRESH = 2;
    constexpr UINT TRAY_CMD_EXIT = 3;
}

namespace wpcc
{
    Application::Application(HINSTANCE instance, int showCommand)
        : m_instance(instance), m_showCommand(showCommand)
    {
    }

    Application::~Application()
    {
        m_trayIcon.Destroy();

        if (m_webViewHost)
        {
            m_webViewHost->Shutdown();
            m_webViewHost.reset();
        }

        m_window.Destroy();

        if (m_comInitialized)
        {
            CoUninitialize();
            m_comInitialized = false;
        }
    }

    bool Application::Initialize()
    {
        const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(comResult))
        {
            MessageBoxW(nullptr, L"Failed to initialize COM for WebView2.", L"Startup error", MB_ICONERROR | MB_OK);
            return false;
        }
        m_comInitialized = true;

        m_window.SetMessageHandler([this](HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, bool& handled) {
            return HandleWindowMessage(hwnd, message, wParam, lParam, handled);
        });

        if (!m_window.Create(m_instance, L"Windows Process Control Center", 1280, 800))
        {
            return false;
        }

        m_webViewHost = std::make_unique<WebViewHost>();
        if (!m_webViewHost->Initialize(m_window.GetHandle()))
        {
            return false;
        }

        m_trayIcon.Create(m_window.GetHandle(), m_instance, IDI_APP_ICON, L"Windows Process Control Center");

        m_window.Show(m_showCommand);
        m_running = true;
        return true;
    }

    int Application::Run()
    {
        MSG message{};
        while (m_running && GetMessageW(&message, nullptr, 0U, 0U) > 0)
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        return static_cast<int>(message.wParam);
    }

    LRESULT Application::HandleWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
    {
        UNREFERENCED_PARAMETER(hwnd);

        switch (message)
        {
        case WebViewHost::ChooseExecutableWindowMessage:
            if (m_webViewHost)
            {
                m_webViewHost->ChooseExecutable();
            }
            handled = true;
            return 0;
        case WM_SIZE:
            if (m_webViewHost && wParam != SIZE_MINIMIZED)
            {
                m_webViewHost->Resize();
            }
            handled = true;
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)
            {
                handled = true;
                return 0;
            }
            break;
        case WM_TRAY_NOTIFY:
            HandleTrayNotification(wParam, lParam);
            handled = true;
            return 0;
        case WM_COMMAND:
            if (lParam == 0)
            {
                const UINT cmd = LOWORD(wParam);
                switch (cmd)
                {
                case TRAY_CMD_OPEN:
                    RestoreMainWindow();
                    handled = true;
                    return 0;
                case TRAY_CMD_REFRESH:
                    if (m_webViewHost)
                    {
                        m_webViewHost->RefreshProcesses();
                    }
                    handled = true;
                    return 0;
                case TRAY_CMD_EXIT:
                    DestroyWindow(m_window.GetHandle());
                    handled = true;
                    return 0;
                }
            }
            break;
        case WM_DESTROY:
            m_running = false;
            PostQuitMessage(0);
            handled = true;
            return 0;
        default:
            break;
        }

        handled = false;
        return 0;
    }

    void Application::HandleTrayNotification(WPARAM wParam, LPARAM lParam)
    {
        UNREFERENCED_PARAMETER(wParam);

        const UINT notification = LOWORD(lParam);

        switch (notification)
        {
        case WM_LBUTTONDBLCLK:
        case 0x400:
            RestoreMainWindow();
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayContextMenu();
            break;
        }
    }

    void Application::ShowTrayContextMenu()
    {
        HMENU menu = CreatePopupMenu();
        if (menu == nullptr)
        {
            return;
        }

        AppendMenuW(menu, MF_STRING, TRAY_CMD_OPEN, L"Open Windows Process Control Center");
        AppendMenuW(menu, MF_STRING, TRAY_CMD_REFRESH, L"Refresh process snapshot");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, TRAY_CMD_EXIT, L"Exit");

        SetForegroundWindow(m_window.GetHandle());

        POINT cursor{};
        GetCursorPos(&cursor);

        TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
            cursor.x, cursor.y, 0, m_window.GetHandle(), nullptr);

        DestroyMenu(menu);
    }

    void Application::RestoreMainWindow()
    {
        HWND hwnd = m_window.GetHandle();
        if (hwnd == nullptr)
        {
            return;
        }

        if (IsIconic(hwnd))
        {
            ShowWindow(hwnd, SW_RESTORE);
        }

        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
    }
}
