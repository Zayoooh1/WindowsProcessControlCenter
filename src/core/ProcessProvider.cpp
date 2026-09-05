#include "core/ProcessProvider.h"

#include <Windows.h>
#include <TlHelp32.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <mutex>
#include <string>
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

    std::string ClassifyOpenProcessFailure(DWORD errorCode, bool systemLikeProcess)
    {
        if (systemLikeProcess)
        {
            return "Protected/System";
        }

        if (errorCode == ERROR_ACCESS_DENIED)
        {
            return "Access denied";
        }

        if (errorCode == ERROR_INVALID_PARAMETER || errorCode == ERROR_NOT_FOUND)
        {
            return "Exited/race";
        }

        return "Unknown/error";
    }

    std::string NormalizeProcessName(std::string_view name)
    {
        std::string result(name);
        result.erase(0, result.find_first_not_of(" \t\r\n"));
        result.erase(result.find_last_not_of(" \t\r\n") + 1);
        for (char& c : result)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return result;
    }

    struct PerformanceCoreTopology
    {
        bool known = false;
        std::vector<DWORD_PTR> masksByGroup;
    };

    PerformanceCoreTopology DetectPerformanceCoreTopology()
    {
        DWORD bufferSize = 0;
        if (GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bufferSize) ||
            GetLastError() != ERROR_INSUFFICIENT_BUFFER || bufferSize == 0)
        {
            return {};
        }

        std::vector<BYTE> buffer(bufferSize);
        if (!GetLogicalProcessorInformationEx(
                RelationProcessorCore,
                reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()),
                &bufferSize))
        {
            return {};
        }

        BYTE highestEfficiencyClass = 0;
        bool foundCore = false;
        const BYTE* current = buffer.data();
        const BYTE* const end = buffer.data() + bufferSize;
        while (current < end)
        {
            const auto* information = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(current);
            if (information->Size == 0 || current + information->Size > end)
            {
                return {};
            }

            if (information->Relationship == RelationProcessorCore)
            {
                highestEfficiencyClass = foundCore
                    ? std::max(highestEfficiencyClass, information->Processor.EfficiencyClass)
                    : information->Processor.EfficiencyClass;
                foundCore = true;
            }

            current += information->Size;
        }

        if (!foundCore)
        {
            return {};
        }

        PerformanceCoreTopology topology{};
        current = buffer.data();
        while (current < end)
        {
            const auto* information = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(current);
            if (information->Relationship == RelationProcessorCore &&
                information->Processor.EfficiencyClass == highestEfficiencyClass)
            {
                for (WORD index = 0; index < information->Processor.GroupCount; ++index)
                {
                    const GROUP_AFFINITY& affinity = information->Processor.GroupMask[index];
                    if (topology.masksByGroup.size() <= affinity.Group)
                    {
                        topology.masksByGroup.resize(static_cast<size_t>(affinity.Group) + 1, 0);
                    }
                    topology.masksByGroup[affinity.Group] |= static_cast<DWORD_PTR>(affinity.Mask);
                }
            }

            current += information->Size;
        }

        topology.known = !topology.masksByGroup.empty();
        return topology;
    }

    const PerformanceCoreTopology& GetPerformanceCoreTopology()
    {
        static std::once_flag initializationFlag;
        static PerformanceCoreTopology topology;
        std::call_once(initializationFlag, [] {
            topology = DetectPerformanceCoreTopology();
        });
        return topology;
    }

    bool TryGetSingleProcessGroup(HANDLE processHandle, USHORT& processGroup)
    {
        if (GetActiveProcessorGroupCount() <= 1)
        {
            processGroup = 0;
            return true;
        }

        // SetProcessAffinityMask remains a single-group operation in this app.
        USHORT groupCount = 1;
        if (!GetProcessGroupAffinity(processHandle, &groupCount, &processGroup) || groupCount != 1)
        {
            return false;
        }

        return true;
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
                process.accessStatus = ClassifyOpenProcessFailure(errorCode, systemLikeProcess);
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
            const int cmp = _stricmp(left.name.c_str(), right.name.c_str());
            if (cmp != 0)
            {
                return cmp < 0;
            }
            return left.pid < right.pid;
        });

        return processes;
    }

    std::vector<ProcessInfo> ProcessProvider::LoadProcessNames() const
    {
        std::vector<ProcessInfo> processes;
        UniqueHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
        if (!snapshot.IsValid())
        {
            return processes;
        }

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (!Process32FirstW(snapshot.Get(), &entry))
        {
            return processes;
        }

        std::unordered_map<unsigned long, std::string> processNameCache;
        processNameCache.reserve(256);

        do
        {
            ProcessInfo process{};
            process.pid = entry.th32ProcessID;
            process.name = WideToUtf8(entry.szExeFile);
            process.accessStatus = "Not checked";
            processNameCache.emplace(process.pid, process.name);
            processes.push_back(std::move(process));
        } while (Process32NextW(snapshot.Get(), &entry));

        {
            std::lock_guard lock(m_processNameCacheMutex);
            m_processNameCache = std::move(processNameCache);
        }

        return processes;
    }

    ProcessInfo ProcessProvider::GetProcess(unsigned long pid) const
    {
        ProcessInfo process{};
        process.pid = pid;
        {
            std::lock_guard lock(m_processNameCacheMutex);
            const auto cachedName = m_processNameCache.find(pid);
            if (cachedName != m_processNameCache.end())
            {
                process.name = cachedName->second;
            }
        }
        process.cpuPriority = "Unknown";
        process.accessStatus = "Unknown";

        if (pid <= 4)
        {
            process.name = pid == 4 ? "System" : "Unknown";
            process.accessStatus = "Protected/System";
            process.likelyRequiresAdmin = true;
            return process;
        }

        UniqueHandle processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
        if (!processHandle.IsValid())
        {
            const DWORD errorCode = GetLastError();
            process.accessError = FormatWin32Error(errorCode);
            process.likelyRequiresAdmin = errorCode == ERROR_ACCESS_DENIED;
            process.accessStatus = ClassifyOpenProcessFailure(errorCode, false);
            return process;
        }

        process.accessStatus = "Accessible";

        DWORD_PTR processAffinityMask = 0;
        DWORD_PTR systemAffinityMask = 0;
        if (GetProcessAffinityMask(processHandle.Get(), &processAffinityMask, &systemAffinityMask))
        {
            process.cpuAffinityMask = static_cast<unsigned long long>(processAffinityMask);
            process.systemAffinityMask = static_cast<unsigned long long>(systemAffinityMask);
            process.cpuAffinityKnown = true;

            USHORT processGroup = 0;
            const PerformanceCoreTopology& topology = GetPerformanceCoreTopology();
            if (topology.known && TryGetSingleProcessGroup(processHandle.Get(), processGroup) &&
                processGroup < topology.masksByGroup.size())
            {
                const DWORD_PTR usablePerformanceMask = topology.masksByGroup[processGroup] & systemAffinityMask;
                if (usablePerformanceMask != 0)
                {
                    process.performanceCoreMask = static_cast<unsigned long long>(usablePerformanceMask);
                    process.performanceCoreMaskKnown = true;
                }
            }
        }

        process.executablePath = QueryExecutablePath(processHandle.Get());
        if (process.executablePath.empty())
        {
            process.accessStatus = "Limited access";
        }
        if (process.name.empty())
        {
            process.name = "Unknown";
            process.accessStatus = "Limited access";
        }

        const DWORD priorityClass = GetPriorityClass(processHandle.Get());
        if (priorityClass == 0)
        {
            process.accessError = FormatWin32Error(GetLastError());
            process.accessStatus = "Limited access";
        }
        else
        {
            process.cpuPriority = PriorityClassToText(priorityClass);
        }

        return process;
    }

    std::vector<unsigned long> ProcessProvider::GetCachedProcessIdsMatchingName(std::string_view targetName) const
    {
        std::vector<unsigned long> pids;
        std::lock_guard lock(m_processNameCacheMutex);
        pids.reserve(m_processNameCache.size());
        for (const auto& [pid, processName] : m_processNameCache)
        {
            if (MatchesProcessName(processName, targetName))
            {
                pids.push_back(pid);
            }
        }

        std::sort(pids.begin(), pids.end());
        return pids;
    }

    bool ProcessProvider::MatchesProcessName(std::string_view processName, std::string_view targetName)
    {
        const std::string normalizedProcessName = NormalizeProcessName(processName);
        const std::string normalizedTargetName = NormalizeProcessName(targetName);
        if (normalizedProcessName.empty() || normalizedTargetName.empty())
        {
            return false;
        }
        if (normalizedProcessName == normalizedTargetName)
        {
            return true;
        }
        if (normalizedTargetName.size() < 4 || normalizedTargetName.substr(normalizedTargetName.size() - 4) != ".exe")
        {
            return normalizedProcessName == normalizedTargetName + ".exe";
        }

        return false;
    }
}
