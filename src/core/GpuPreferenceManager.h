#pragma once

#include "core/ProcessActions.h"

#include <string>
#include <string_view>

namespace wpcc
{
    class GpuPreferenceManager
    {
    public:
        std::string GetPreferenceForExecutablePath(const std::string& executablePath) const;
        ProcessActionResult SetPreferenceForProcess(unsigned long pid, const std::string& expectedName, const std::string& executablePath, const std::string& preference) const;

    private:
        static bool IsValidExecutablePath(const std::string& executablePath);
        static std::wstring Utf8ToWide(std::string_view value);
        static std::string WideToUtf8(std::wstring_view value);
        static std::string FormatWin32Error(unsigned long errorCode);
        static std::string RegistryValueToPreference(std::wstring_view value);
        static std::wstring PreferenceToRegistryValue(const std::string& preference);
        static std::string PreferenceToDisplayName(const std::string& preference);
        static bool EqualsIgnoreCase(std::string_view left, std::string_view right);
    };
}
