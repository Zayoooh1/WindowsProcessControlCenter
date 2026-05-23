#include "platform/Win32Window.h"

#include <string>

namespace
{
    constexpr wchar_t WindowClassName[] = L"WPCC.MainWindow";
    constexpr LONG MinWindowWidth = 1060;
    constexpr LONG MinWindowHeight = 680;

    void EnableDpiAwareness()
    {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }
}

namespace wpcc
{
    Win32Window::~Win32Window()
    {
        Destroy();
    }

    bool Win32Window::Create(HINSTANCE instance, std::wstring_view title, int width, int height)
    {
        EnableDpiAwareness();

        m_instance = instance;

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.style = CS_CLASSDC;
        windowClass.lpfnWndProc = StaticWindowProc;
        windowClass.hInstance = m_instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.lpszClassName = WindowClassName;

        if (!RegisterClassExW(&windowClass))
        {
            return false;
        }

        RECT rect{0, 0, width, height};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        const std::wstring windowTitle(title);

        m_hwnd = CreateWindowW(
            WindowClassName,
            windowTitle.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            m_instance,
            this);

        return m_hwnd != nullptr;
    }

    void Win32Window::Show(int showCommand) const
    {
        ShowWindow(m_hwnd, showCommand);
        UpdateWindow(m_hwnd);
    }

    void Win32Window::Destroy()
    {
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }

        if (m_instance)
        {
            UnregisterClassW(WindowClassName, m_instance);
            m_instance = nullptr;
        }
    }

    void Win32Window::SetMessageHandler(MessageHandler handler)
    {
        m_messageHandler = std::move(handler);
    }

    LRESULT CALLBACK Win32Window::StaticWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Win32Window* window = nullptr;

        if (message == WM_NCCREATE)
        {
            const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            window = static_cast<Win32Window*>(createStruct->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        }
        else
        {
            window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (window)
        {
            return window->WindowProc(hwnd, message, wParam, lParam);
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    LRESULT Win32Window::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_GETMINMAXINFO)
        {
            auto* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
            minMaxInfo->ptMinTrackSize.x = MinWindowWidth;
            minMaxInfo->ptMinTrackSize.y = MinWindowHeight;
            return 0;
        }

        if (message == WM_NCDESTROY)
        {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            m_hwnd = nullptr;
        }

        if (m_messageHandler)
        {
            bool handled = false;
            const LRESULT result = m_messageHandler(hwnd, message, wParam, lParam, handled);
            if (handled)
            {
                return result;
            }
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}
