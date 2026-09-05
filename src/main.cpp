#include "app/Application.h"

#include <Windows.h>

#include <cstdlib>

namespace
{
    class ScopedHandle
    {
    public:
        explicit ScopedHandle(HANDLE value) : m_value(value) {}
        ~ScopedHandle()
        {
            if (m_value != nullptr && m_value != INVALID_HANDLE_VALUE)
            {
                CloseHandle(m_value);
            }
        }

        ScopedHandle(const ScopedHandle&) = delete;
        ScopedHandle& operator=(const ScopedHandle&) = delete;

    private:
        HANDLE m_value;
    };
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmdLine, int showCommand)
{
    constexpr wchar_t SingleInstanceMutexName[] = L"Local\\WindowsProcessControlCenter.SingleInstance";
    HANDLE singleInstanceMutex = CreateMutexW(nullptr, FALSE, SingleInstanceMutexName);
    if (singleInstanceMutex == nullptr)
    {
        MessageBoxW(nullptr, L"Windows Process Control Center could not create its single-instance guard.",
            L"Windows Process Control Center", MB_ICONERROR | MB_OK);
        return EXIT_FAILURE;
    }
    ScopedHandle singleInstanceGuard(singleInstanceMutex);

    const DWORD mutexError = GetLastError();
    if (mutexError == ERROR_ALREADY_EXISTS)
    {
        MessageBoxW(nullptr,
            L"Windows Process Control Center is already running.\n\nOnly one instance of the application can run at a time.",
            L"Windows Process Control Center", MB_ICONINFORMATION | MB_OK);
        return EXIT_SUCCESS;
    }

    bool startMinimized = cmdLine && wcsstr(cmdLine, L"--minimized") != nullptr;
    wpcc::Application app(instance, showCommand, startMinimized);
    if (!app.Initialize())
    {
        MessageBoxW(nullptr, L"Windows Process Control Center failed to initialize.", L"Startup error", MB_ICONERROR | MB_OK);
        return EXIT_FAILURE;
    }

    return app.Run();
}
