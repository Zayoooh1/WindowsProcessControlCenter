#include "app/Application.h"

#include <Windows.h>
#include <objbase.h>

namespace wpcc
{
    Application::Application(HINSTANCE instance, int showCommand)
        : m_instance(instance), m_showCommand(showCommand)
    {
    }

    Application::~Application()
    {
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
        UNREFERENCED_PARAMETER(lParam);

        switch (message)
        {
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
}
