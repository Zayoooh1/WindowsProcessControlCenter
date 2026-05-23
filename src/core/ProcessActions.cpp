#include "core/ProcessActions.h"

#include "core/ProcessProvider.h"

#include <Windows.h>

#include <algorithm>
#include <string_view>
#include <vector>

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
            if (m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(m_handle);
            }
        }

        UniqueHandle(const UniqueHandle&) = delete;
        UniqueHandle& operator=(const UniqueHandle&) = delete;

        HANDLE Get() const
        {
            return m_handle;
        }

        bool IsValid() const
        {
            return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE;
        }

    private:
        HANDLE m_handle = nullptr;
    };
}

namespace wpcc
{
    ProcessActionResult ProcessActions::SetCpuPriority(unsigned long pid, const std::string& priority, bool confirmRealtime) const
    {
        ProcessActionResult result{};
        result.pid = pid;

        if (pid == 0)
        {
            result.message = "PID 0 cannot be modified.";
            return result;
        }

        if (pid == GetCurrentProcessId())
        {
            result.message = "Changing this application's own priority is blocked.";
            return result;
        }

        const DWORD priorityClass = PriorityTextToClass(priority);
        if (priorityClass == 0)
        {
            result.message = "Unsupported CPU priority.";
            return result;
        }

        if (priorityClass == REALTIME_PRIORITY_CLASS && !confirmRealtime)
        {
            result.message = "Realtime priority requires explicit risk confirmation.";
            return result;
        }

        const ProcessProvider provider;
        const std::vector<ProcessInfo> processes = provider.LoadProcesses();
        const auto processIt = std::find_if(processes.begin(), processes.end(), [pid](const ProcessInfo& process) {
            return process.pid == pid;
        });

        if (processIt == processes.end())
        {
            result.message = "The process is no longer running.";
            result.win32ErrorCode = ERROR_NOT_FOUND;
            return result;
        }

        if (processIt->name == "System" || processIt->accessStatus == "Protected/System")
        {
            result.message = "Protected or system processes cannot be modified.";
            return result;
        }

        if (processIt->accessStatus == "Access denied")
        {
            result.message = "Access denied. Administrator permissions may be required.";
            result.win32ErrorCode = ERROR_ACCESS_DENIED;
            return result;
        }

        if (processIt->accessStatus != "Accessible")
        {
            result.message = "This process is not accessible.";
            return result;
        }

        UniqueHandle processHandle(OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
        if (!processHandle.IsValid())
        {
            const DWORD errorCode = GetLastError();
            result.win32ErrorCode = errorCode;
            result.message = errorCode == ERROR_ACCESS_DENIED ? "Access denied. Administrator permissions may be required." : FormatWin32Error(errorCode);
            return result;
        }

        if (!SetPriorityClass(processHandle.Get(), priorityClass))
        {
            const DWORD errorCode = GetLastError();
            result.win32ErrorCode = errorCode;
            result.message = FormatWin32Error(errorCode);
            return result;
        }

        result.success = true;
        result.message = "CPU priority changed successfully.";
        return result;
    }

    unsigned long ProcessActions::PriorityTextToClass(const std::string& priority)
    {
        if (priority == "Realtime")
        {
            return REALTIME_PRIORITY_CLASS;
        }
        if (priority == "High")
        {
            return HIGH_PRIORITY_CLASS;
        }
        if (priority == "Above Normal" || priority == "Above normal")
        {
            return ABOVE_NORMAL_PRIORITY_CLASS;
        }
        if (priority == "Normal")
        {
            return NORMAL_PRIORITY_CLASS;
        }
        if (priority == "Below Normal" || priority == "Below normal")
        {
            return BELOW_NORMAL_PRIORITY_CLASS;
        }
        if (priority == "Idle")
        {
            return IDLE_PRIORITY_CLASS;
        }

        return 0;
    }

    std::string ProcessActions::FormatWin32Error(unsigned long errorCode)
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

        std::wstring wideMessage(buffer, size);
        LocalFree(buffer);
        while (!wideMessage.empty() && (wideMessage.back() == L'\r' || wideMessage.back() == L'\n' || wideMessage.back() == L'.' || wideMessage.back() == L' '))
        {
            wideMessage.pop_back();
        }

        const int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wideMessage.c_str(), static_cast<int>(wideMessage.size()), nullptr, 0, nullptr, nullptr);
        if (requiredSize <= 0)
        {
            return "Win32 error " + std::to_string(errorCode);
        }

        std::string message(static_cast<size_t>(requiredSize), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wideMessage.c_str(), static_cast<int>(wideMessage.size()), message.data(), requiredSize, nullptr, nullptr);
        return message + " (" + std::to_string(errorCode) + ")";
    }
}
