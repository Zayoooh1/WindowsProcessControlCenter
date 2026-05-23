#include "core/GpuPreferenceManager.h"

#include "core/ProcessProvider.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    constexpr wchar_t GpuPreferencesSubKey[] = L"Software\\Microsoft\\DirectX\\UserGpuPreferences";
}

namespace wpcc
{
    std::string GpuPreferenceManager::GetPreferenceForExecutablePath(const std::string& executablePath) const
    {
        if (!IsValidExecutablePath(executablePath))
        {
            return "Unknown";
        }

        const std::wstring valueName = Utf8ToWide(executablePath);
        std::wstring buffer(512, L'\0');
        DWORD bufferSizeBytes = static_cast<DWORD>(buffer.size() * sizeof(wchar_t));
        const LSTATUS status = RegGetValueW(
            HKEY_CURRENT_USER,
            GpuPreferencesSubKey,
            valueName.c_str(),
            RRF_RT_REG_SZ,
            nullptr,
            buffer.data(),
            &bufferSizeBytes);

        if (status == ERROR_FILE_NOT_FOUND)
        {
            return "SystemDefault";
        }

        if (status != ERROR_SUCCESS || bufferSizeBytes < sizeof(wchar_t))
        {
            return "Unknown";
        }

        const size_t characterCount = (bufferSizeBytes / sizeof(wchar_t));
        if (characterCount > 0 && buffer[characterCount - 1] == L'\0')
        {
            buffer.resize(characterCount - 1);
        }
        else
        {
            buffer.resize(characterCount);
        }

        return RegistryValueToPreference(buffer);
    }

    ProcessActionResult GpuPreferenceManager::SetPreferenceForProcess(
        unsigned long pid,
        const std::string& expectedName,
        const std::string& executablePath,
        const std::string& preference) const
    {
        ProcessActionResult result{};
        result.pid = pid;
        result.currentPreference = GetPreferenceForExecutablePath(executablePath);

        if (pid == 0 || pid == 4)
        {
            result.message = "System process GPU preference cannot be changed here.";
            return result;
        }

        if (!IsValidExecutablePath(executablePath))
        {
            result.message = "Executable path is unavailable.";
            result.currentPreference = "Unknown";
            return result;
        }

        const std::wstring registryValue = PreferenceToRegistryValue(preference);
        if (registryValue.empty() && preference != "SystemDefault")
        {
            result.message = "Unsupported GPU preference.";
            return result;
        }

        const ProcessProvider provider;
        const std::vector<ProcessInfo> processes = provider.LoadProcesses();
        const auto processIt = std::find_if(processes.begin(), processes.end(), [pid](const ProcessInfo& process) {
            return process.pid == pid;
        });

        if (processIt == processes.end())
        {
            result.message = "Process is no longer running.";
            result.win32ErrorCode = ERROR_NOT_FOUND;
            return result;
        }

        if (!expectedName.empty() && !EqualsIgnoreCase(processIt->name, expectedName))
        {
            result.message = "Process identity changed. Refresh the process list and try again.";
            return result;
        }

        if (processIt->accessStatus == "Protected/System")
        {
            result.message = "Protected/system process GPU preference cannot be changed here.";
            return result;
        }

        if (processIt->accessStatus == "Access denied" && !IsValidExecutablePath(processIt->executablePath))
        {
            result.message = "Executable path is unavailable.";
            result.currentPreference = "Unknown";
            return result;
        }

        if (!EqualsIgnoreCase(processIt->executablePath, executablePath))
        {
            result.message = "Executable path changed. Refresh the process list and try again.";
            return result;
        }

        HKEY rawKey = nullptr;
        LSTATUS status = RegCreateKeyExW(
            HKEY_CURRENT_USER,
            GpuPreferencesSubKey,
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            KEY_QUERY_VALUE | KEY_SET_VALUE,
            nullptr,
            &rawKey,
            nullptr);

        if (status != ERROR_SUCCESS)
        {
            result.win32ErrorCode = status;
            result.message = "Failed to open GPU preference registry key: " + FormatWin32Error(status);
            return result;
        }

        const auto closeKey = [&rawKey]() {
            if (rawKey != nullptr)
            {
                RegCloseKey(rawKey);
                rawKey = nullptr;
            }
        };

        const std::wstring valueName = Utf8ToWide(executablePath);
        if (preference == "SystemDefault")
        {
            status = RegDeleteValueW(rawKey, valueName.c_str());
            if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND)
            {
                closeKey();
                result.win32ErrorCode = status;
                result.message = "Failed to reset GPU preference: " + FormatWin32Error(status);
                return result;
            }

            closeKey();
            result.success = true;
            result.currentPreference = "SystemDefault";
            result.message = "GPU preference reset to System default.";
            return result;
        }

        // Windows Graphics Settings stores per-user preferences as REG_SZ values:
        // GpuPreference=1; means Power saving, GpuPreference=2; means High performance.
        status = RegSetValueExW(
            rawKey,
            valueName.c_str(),
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(registryValue.c_str()),
            static_cast<DWORD>((registryValue.size() + 1) * sizeof(wchar_t)));
        closeKey();

        if (status != ERROR_SUCCESS)
        {
            result.win32ErrorCode = status;
            result.message = "Failed to write GPU preference to registry: " + FormatWin32Error(status);
            return result;
        }

        result.success = true;
        result.currentPreference = preference;
        result.message = "GPU preference set to " + PreferenceToDisplayName(preference) + ". Restart the target app to apply it.";
        return result;
    }

    bool GpuPreferenceManager::IsValidExecutablePath(const std::string& executablePath)
    {
        if (executablePath.empty() || executablePath == "Unavailable")
        {
            return false;
        }

        constexpr std::string_view exeSuffix = ".exe";
        if (executablePath.size() < exeSuffix.size())
        {
            return false;
        }

        return EqualsIgnoreCase(std::string_view(executablePath).substr(executablePath.size() - exeSuffix.size()), exeSuffix);
    }

    std::wstring GpuPreferenceManager::Utf8ToWide(std::string_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int requiredSize = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (requiredSize <= 0)
        {
            return {};
        }

        std::wstring result(static_cast<size_t>(requiredSize), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), requiredSize);
        return result;
    }

    std::string GpuPreferenceManager::WideToUtf8(std::wstring_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int requiredSize = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (requiredSize <= 0)
        {
            return {};
        }

        std::string result(static_cast<size_t>(requiredSize), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), requiredSize, nullptr, nullptr);
        return result;
    }

    std::string GpuPreferenceManager::FormatWin32Error(unsigned long errorCode)
    {
        wchar_t* buffer = nullptr;
        const DWORD size = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&buffer),
            0,
            nullptr);

        if (size == 0 || buffer == nullptr)
        {
            return "Win32 error " + std::to_string(errorCode);
        }

        std::wstring message(buffer, size);
        LocalFree(buffer);
        while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n' || message.back() == L'.' || message.back() == L' '))
        {
            message.pop_back();
        }

        return WideToUtf8(message) + " (" + std::to_string(errorCode) + ")";
    }

    std::string GpuPreferenceManager::RegistryValueToPreference(std::wstring_view value)
    {
        if (value.find(L"GpuPreference=1;") != std::wstring_view::npos)
        {
            return "PowerSaving";
        }

        if (value.find(L"GpuPreference=2;") != std::wstring_view::npos)
        {
            return "HighPerformance";
        }

        return "Unknown";
    }

    std::wstring GpuPreferenceManager::PreferenceToRegistryValue(const std::string& preference)
    {
        if (preference == "PowerSaving")
        {
            return L"GpuPreference=1;";
        }

        if (preference == "HighPerformance")
        {
            return L"GpuPreference=2;";
        }

        return {};
    }

    std::string GpuPreferenceManager::PreferenceToDisplayName(const std::string& preference)
    {
        if (preference == "PowerSaving")
        {
            return "Power saving";
        }

        if (preference == "HighPerformance")
        {
            return "High performance";
        }

        return "System default";
    }

    bool GpuPreferenceManager::EqualsIgnoreCase(std::string_view left, std::string_view right)
    {
        if (left.size() != right.size())
        {
            return false;
        }

        return std::equal(left.begin(), left.end(), right.begin(), [](unsigned char lhs, unsigned char rhs) {
            return std::tolower(lhs) == std::tolower(rhs);
        });
    }
}
