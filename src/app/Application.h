#pragma once

#include "platform/Win32Window.h"
#include "ui_web/WebViewHost.h"

#include <Windows.h>

#include <memory>

namespace wpcc
{
    class Application
    {
    public:
        Application(HINSTANCE instance, int showCommand);
        ~Application();

        bool Initialize();
        int Run();

    private:
        LRESULT HandleWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, bool& handled);

        HINSTANCE m_instance;
        int m_showCommand;
        Win32Window m_window;
        std::unique_ptr<WebViewHost> m_webViewHost;
        bool m_comInitialized = false;
        bool m_running = false;
    };
}
