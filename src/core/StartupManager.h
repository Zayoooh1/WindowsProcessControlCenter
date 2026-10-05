#pragma once
#include <Windows.h>
#include <string>

namespace wpcc
{
    struct StartupState
    {
        bool known = false;
        bool enabled = false; // Verified task, enabled, with the current executable/configuration.
        bool legacy = false;  // Exact old Run command for this executable (never treated as working startup).
        bool present = false;
        HRESULT error = S_OK;
        std::wstring warning;
    };

    // Calls require COM on the calling thread. Never registers during a read.
    class StartupManager
    {
    public:
        static StartupState Read();
        static StartupState SetEnabled(bool enable);
        // Only migrates an existing, enabled legacy Run entry; missing/disabled entries stay disabled.
        static StartupState MigrateLegacy(bool savedIntent);
        // Used before uninstall: all user tasks and legacy values for this installation only.
        static HRESULT RemoveInstallationStartup();
        static std::wstring ExecutablePath();
    };
    void StartupLog(const wchar_t* stage, HRESULT result = S_OK) noexcept;
}
