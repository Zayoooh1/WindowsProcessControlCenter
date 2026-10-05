#include "app/Application.h"
#include "core/StartupManager.h"
#include <shellapi.h>
#include <string_view>

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
    (void)cmdLine;
    wpcc::StartupLog(L"process.entry");
    bool startMinimized = false;
    bool cleanup = false;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return EXIT_FAILURE;
    for (int i = 1; i < argc; ++i)
    {
        startMinimized |= std::wstring_view(argv[i]) == L"--minimized";
        cleanup |= std::wstring_view(argv[i]) == L"--remove-installation-startup";
    }
    LocalFree(argv);
    // Installer maintenance runs before the GUI mutex, even if WPCC is already open.
    if (cleanup)
    {
        const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(com)) return EXIT_FAILURE;
        const HRESULT security = CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
        const HRESULT result = (SUCCEEDED(security) || security == RPC_E_TOO_LATE)
            ? wpcc::StartupManager::RemoveInstallationStartup() : security;
        CoUninitialize();
        return SUCCEEDED(result) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
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
        if (!startMinimized) MessageBoxW(nullptr,
            L"Windows Process Control Center is already running.\n\nOnly one instance of the application can run at a time.",
            L"Windows Process Control Center", MB_ICONINFORMATION | MB_OK);
        return EXIT_SUCCESS;
    }

    wpcc::Application app(instance, showCommand, startMinimized);
    if (!app.Initialize())
    {
        MessageBoxW(nullptr, L"Windows Process Control Center failed to initialize.", L"Startup error", MB_ICONERROR | MB_OK);
        return EXIT_FAILURE;
    }

    return app.Run();
}
