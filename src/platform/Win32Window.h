#pragma once

#include <Windows.h>

#include <functional>
#include <string_view>

namespace wpcc
{
    class Win32Window
    {
    public:
        using MessageHandler = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM, bool&)>;

        Win32Window() = default;
        ~Win32Window();

        Win32Window(const Win32Window&) = delete;
        Win32Window& operator=(const Win32Window&) = delete;

        bool Create(HINSTANCE instance, std::wstring_view title, int width, int height);
        void Show(int showCommand) const;
        void Destroy();

        HWND GetHandle() const { return m_hwnd; }
        void SetMessageHandler(MessageHandler handler);

    private:
        static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

        HINSTANCE m_instance = nullptr;
        HWND m_hwnd = nullptr;
        MessageHandler m_messageHandler;
    };
}
