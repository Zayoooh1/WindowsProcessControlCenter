#include "core/ProcessProvider.h"

#include <Windows.h>
#include <TlHelp32.h>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace
{
    class UniqueHandle
    {
    public:
        explicit UniqueHandle(HANDLE handle = nullptr)
            : m_handle(handle)
        {
        }

        ~UniqueHandle()
        {
            if (IsValid())
            {
                CloseHandle(m_handle);
            }
        }

        UniqueHandle(const UniqueHandle&) = delete;
        UniqueHandle& operator=(const UniqueHandle&) = delete;

        UniqueHandle(UniqueHandle&& other) noexcept
            : m_handle(other.m_handle)
        {
            other.m_handle = nullptr;
        }

        UniqueHandle& operator=(UniqueHandle&& other) noexcept
        {
            if (this != &other)
            {
                if (IsValid())
                {
                    CloseHandle(m_handle);
                }

                m_handle = other.m_handle;
                other.m_handle = nullptr;
            }

            return *this;
        }

        bool IsValid() const
        {
            return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE;
        }

        HANDLE Get() const
        {
            return m_handle;
        }

    private:
        HANDLE m_handle = nullptr;
    };

    std::string WideToUtf8(std::wstring_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int requiredSize = WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (requiredSize <= 0)
        {
            return {};
        }

        std::string result(static_cast<size_t>(requiredSize), '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            result.data(),
            requiredSize,
            nullptr,
            nullptr);

        return result;
    }

    std::string FormatWin32Error(DWORD errorCode)
    {
        if (errorCode == ERROR_SUCCESS)
        {
            return {};
        }

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

    bool IsSystemLikeProcess(DWORD pid, std::wstring_view processName)
    {
        if (pid <= 4)
        {
            return true;
        }

        constexpr std::wstring_view protectedNames[] = {
            L"System",
            L"Registry",
            L"Secure System",
            L"Memory Compression",
        };

        return std::any_of(std::begin(protectedNames), std::end(protectedNames), [processName](std::wstring_view protectedName) {
            return processName == protectedName;
        });
    }

    std::string PriorityClassToText(DWORD priorityClass)
    {
        switch (priorityClass)
        {
        case IDLE_PRIORITY_CLASS:
            return "Idle";
        case BELOW_NORMAL_PRIORITY_CLASS:
            return "Below normal";
        case NORMAL_PRIORITY_CLASS:
            return "Normal";
        case ABOVE_NORMAL_PRIORITY_CLASS:
            return "Above normal";
        case HIGH_PRIORITY_CLASS:
            return "High";
        case REALTIME_PRIORITY_CLASS:
            return "Realtime";
        default:
            return "Unknown";
        }
    }

    std::string QueryExecutablePath(HANDLE processHandle)
    {
        std::array<wchar_t, 32768> pathBuffer{};
        DWORD size = static_cast<DWORD>(pathBuffer.size());

        if (!QueryFullProcessImageNameW(processHandle, 0, pathBuffer.data(), &size))
        {
            return {};
        }

        return WideToUtf8(std::wstring_view(pathBuffer.data(), size));
    }
}

namespace wpcc
{
    std::vector<ProcessInfo> ProcessProvider::LoadProcesses() const
    {
        std::vector<ProcessInfo> processes;

        UniqueHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
        if (!snapshot.IsValid())
        {
            return processes;
        }

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(PROCESSENTRY32W);

        if (!Process32FirstW(snapshot.Get(), &entry))
        {
            return processes;
        }

        do
        {
            ProcessInfo process{};
            process.pid = entry.th32ProcessID;
            process.name = WideToUtf8(entry.szExeFile);
            process.cpuPriority = "Unknown";
            process.accessStatus = "Unknown";

            const bool systemLikeProcess = IsSystemLikeProcess(entry.th32ProcessID, entry.szExeFile);
            UniqueHandle processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID));

            if (!processHandle.IsValid())
            {
                const DWORD errorCode = GetLastError();
                process.accessError = FormatWin32Error(errorCode);
                process.likelyRequiresAdmin = errorCode == ERROR_ACCESS_DENIED || systemLikeProcess;
                process.accessStatus = systemLikeProcess ? "Protected/System" : (errorCode == ERROR_ACCESS_DENIED ? "Access denied" : "Unknown");
            }
            else
            {
                process.accessStatus = "Accessible";
                process.executablePath = QueryExecutablePath(processHandle.Get());

                const DWORD priorityClass = GetPriorityClass(processHandle.Get());
                if (priorityClass == 0)
                {
                    process.cpuPriority = "Unknown";
                    process.accessError = FormatWin32Error(GetLastError());
                }
                else
                {
                    process.cpuPriority = PriorityClassToText(priorityClass);
                }
            }

            processes.push_back(std::move(process));
        } while (Process32NextW(snapshot.Get(), &entry));

        std::sort(processes.begin(), processes.end(), [](const ProcessInfo& left, const ProcessInfo& right) {
            return left.name < right.name;
        });

        return processes;
    }
}
