#include "core/ProcessActions.h"

#include "core/ProcessProvider.h"

#include <Windows.h>
#include <TlHelp32.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>
#include <sstream>
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

    ProcessActionResult ProcessActions::TerminateProcessByPid(unsigned long pid, const std::string& expectedName, const std::string& confirmation) const
    {
        ProcessActionResult result{};
        result.pid = pid;

        if (pid == 0)
        {
            result.message = "PID 0 cannot be ended.";
            return result;
        }

        if (pid == 4)
        {
            result.message = "The System process cannot be ended.";
            return result;
        }

        if (pid == GetCurrentProcessId())
        {
            result.message = "Cannot end this application from itself.";
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

        if (IsCriticalProcessName(processIt->name))
        {
            result.message = "This is a critical Windows process and cannot be ended here.";
            return result;
        }

        if (processIt->accessStatus == "Protected/System")
        {
            result.message = "Protected/system process cannot be ended.";
            return result;
        }

        if (processIt->accessStatus == "Access denied")
        {
            result.message = "Access denied or protected process.";
            result.win32ErrorCode = ERROR_ACCESS_DENIED;
            return result;
        }

        if (processIt->accessStatus != "Accessible")
        {
            result.message = "This process is not accessible.";
            return result;
        }

        const bool hasProcessName = !processIt->name.empty() && !EqualsIgnoreCase(processIt->name, "Unknown");
        const bool confirmationMatches = hasProcessName ? EqualsIgnoreCase(confirmation, processIt->name) : confirmation == std::to_string(pid);
        if (!confirmationMatches)
        {
            result.message = hasProcessName ? "Confirmation does not match the process name." : "Confirmation does not match the process PID.";
            return result;
        }

        UniqueHandle processHandle(OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid));
        if (!processHandle.IsValid())
        {
            const DWORD errorCode = GetLastError();
            result.win32ErrorCode = errorCode;
            result.message = errorCode == ERROR_ACCESS_DENIED ? "Access denied or protected process." : FormatWin32Error(errorCode);
            return result;
        }

        if (!TerminateProcess(processHandle.Get(), 1))
        {
            const DWORD errorCode = GetLastError();
            result.win32ErrorCode = errorCode;
            result.message = errorCode == ERROR_ACCESS_DENIED ? "Access denied or protected process." : FormatWin32Error(errorCode);
            return result;
        }

        WaitForSingleObject(processHandle.Get(), 1500);

        result.success = true;
        result.message = "Process terminated successfully.";
        return result;
    }

    ProcessActionResult ProcessActions::FreezeProcessByPid(unsigned long pid, const std::string& expectedName, const std::string& confirmation)
    {
        ProcessActionResult result{};
        result.pid = pid;

        if (m_frozenThreadsByPid.contains(pid))
        {
            result.message = "This process is already frozen by this app.";
            return result;
        }

        if (pid == 0)
        {
            result.message = "PID 0 cannot be frozen.";
            return result;
        }

        if (pid == 4)
        {
            result.message = "The System process cannot be frozen.";
            return result;
        }

        if (pid == GetCurrentProcessId())
        {
            result.message = "Cannot freeze this application from itself.";
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

        if (IsCriticalProcessName(processIt->name))
        {
            result.message = "This is a critical Windows process and cannot be frozen here.";
            return result;
        }

        if (processIt->accessStatus == "Protected/System")
        {
            result.message = "Protected/system process cannot be frozen.";
            return result;
        }

        if (processIt->accessStatus == "Access denied")
        {
            result.message = "Access denied or protected process.";
            result.win32ErrorCode = ERROR_ACCESS_DENIED;
            return result;
        }

        if (processIt->accessStatus != "Accessible")
        {
            result.message = "This process is not accessible.";
            return result;
        }

        const bool hasProcessName = !processIt->name.empty() && !EqualsIgnoreCase(processIt->name, "Unknown");
        const bool confirmationMatches = hasProcessName ? EqualsIgnoreCase(confirmation, processIt->name) : confirmation == std::to_string(pid);
        if (!confirmationMatches)
        {
            result.message = hasProcessName ? "Confirmation does not match the process name." : "Confirmation does not match the process PID.";
            return result;
        }

        UniqueHandle threadSnapshot(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0));
        if (!threadSnapshot.IsValid())
        {
            const DWORD errorCode = GetLastError();
            result.win32ErrorCode = errorCode;
            result.message = FormatWin32Error(errorCode);
            return result;
        }

        std::vector<unsigned long> threadIds;
        THREADENTRY32 threadEntry{};
        threadEntry.dwSize = sizeof(THREADENTRY32);
        if (Thread32First(threadSnapshot.Get(), &threadEntry))
        {
            do
            {
                if (threadEntry.th32OwnerProcessID == pid)
                {
                    threadIds.push_back(threadEntry.th32ThreadID);
                }
                threadEntry.dwSize = sizeof(THREADENTRY32);
            } while (Thread32Next(threadSnapshot.Get(), &threadEntry));
        }

        if (threadIds.empty())
        {
            result.message = "No threads were found for this process.";
            return result;
        }

        std::vector<FrozenThreadRecord> frozenThreads;
        unsigned long failedThreads = 0;
        for (unsigned long threadId : threadIds)
        {
            UniqueHandle threadHandle(OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadId));
            if (!threadHandle.IsValid())
            {
                ++failedThreads;
                continue;
            }

            const DWORD previousSuspendCount = SuspendThread(threadHandle.Get());
            if (previousSuspendCount == static_cast<DWORD>(-1))
            {
                ++failedThreads;
                continue;
            }

            frozenThreads.push_back(FrozenThreadRecord{threadId, 1});
        }

        if (frozenThreads.empty())
        {
            result.message = "No process threads could be frozen.";
            result.win32ErrorCode = failedThreads > 0 ? ERROR_ACCESS_DENIED : 0;
            return result;
        }

        m_frozenThreadsByPid[pid] = frozenThreads;

        result.success = true;
        std::ostringstream message;
        message << "Process frozen successfully. Suspended " << frozenThreads.size() << " thread";
        if (frozenThreads.size() != 1)
        {
            message << "s";
        }
        if (failedThreads > 0)
        {
            message << "; " << failedThreads << " thread";
            if (failedThreads != 1)
            {
                message << "s";
            }
            message << " could not be suspended.";
        }
        else
        {
            message << ".";
        }
        result.message = message.str();
        return result;
    }

    ProcessActionResult ProcessActions::ResumeProcessByPid(unsigned long pid, const std::string& expectedName)
    {
        ProcessActionResult result{};
        result.pid = pid;

        const auto frozenIt = m_frozenThreadsByPid.find(pid);
        if (frozenIt == m_frozenThreadsByPid.end())
        {
            result.message = "This process was not frozen by this app.";
            return result;
        }

        const ProcessProvider provider;
        const std::vector<ProcessInfo> processes = provider.LoadProcesses();
        const auto processIt = std::find_if(processes.begin(), processes.end(), [pid](const ProcessInfo& process) {
            return process.pid == pid;
        });

        if (processIt == processes.end())
        {
            m_frozenThreadsByPid.erase(frozenIt);
            result.message = "Process is no longer running.";
            result.win32ErrorCode = ERROR_NOT_FOUND;
            return result;
        }

        if (!expectedName.empty() && !EqualsIgnoreCase(processIt->name, expectedName))
        {
            result.message = "Process identity changed. Refresh the process list and try again.";
            return result;
        }

        unsigned long resumedThreads = 0;
        unsigned long failedThreads = 0;
        for (const FrozenThreadRecord& record : frozenIt->second)
        {
            UniqueHandle threadHandle(OpenThread(THREAD_SUSPEND_RESUME, FALSE, record.threadId));
            if (!threadHandle.IsValid())
            {
                ++failedThreads;
                continue;
            }

            for (unsigned long index = 0; index < record.resumeCount; ++index)
            {
                if (ResumeThread(threadHandle.Get()) == static_cast<DWORD>(-1))
                {
                    ++failedThreads;
                    break;
                }
                ++resumedThreads;
            }
        }

        m_frozenThreadsByPid.erase(frozenIt);

        if (resumedThreads == 0)
        {
            result.message = "No frozen threads could be resumed.";
            result.win32ErrorCode = failedThreads > 0 ? ERROR_ACCESS_DENIED : 0;
            return result;
        }

        result.success = true;
        std::ostringstream message;
        message << "Process resumed successfully. Resumed " << resumedThreads << " thread";
        if (resumedThreads != 1)
        {
            message << "s";
        }
        if (failedThreads > 0)
        {
            message << "; " << failedThreads << " thread";
            if (failedThreads != 1)
            {
                message << "s";
            }
            message << " could not be resumed.";
        }
        else
        {
            message << ".";
        }
        result.message = message.str();
        return result;
    }

    void ProcessActions::ResumeAllFrozenProcesses()
    {
        std::vector<unsigned long> frozenPids;
        frozenPids.reserve(m_frozenThreadsByPid.size());
        for (const auto& [pid, records] : m_frozenThreadsByPid)
        {
            UNREFERENCED_PARAMETER(records);
            frozenPids.push_back(pid);
        }

        for (unsigned long pid : frozenPids)
        {
            ResumeProcessByPid(pid, {});
        }
    }

    bool ProcessActions::IsFrozenByApp(unsigned long pid) const
    {
        return m_frozenThreadsByPid.contains(pid);
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

    bool ProcessActions::IsCriticalProcessName(const std::string& name)
    {
        constexpr std::array<std::string_view, 13> criticalNames = {
            "System",
            "Registry",
            "smss.exe",
            "csrss.exe",
            "wininit.exe",
            "winlogon.exe",
            "services.exe",
            "lsass.exe",
            "svchost.exe",
            "fontdrvhost.exe",
            "dwm.exe",
            "explorer.exe",
            "audiodg.exe",
        };

        return std::any_of(criticalNames.begin(), criticalNames.end(), [&name](std::string_view criticalName) {
            return EqualsIgnoreCase(name, criticalName);
        });
    }

    bool ProcessActions::EqualsIgnoreCase(std::string_view left, std::string_view right)
    {
        if (left.size() != right.size())
        {
            return false;
        }

        return std::equal(left.begin(), left.end(), right.begin(), [](unsigned char lhs, unsigned char rhs) {
            return std::tolower(lhs) == std::tolower(rhs);
        });
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
