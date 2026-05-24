#pragma once

#include "core/ProfileStore.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace wpcc
{
    struct ProcessActionResult
    {
        bool success = false;
        unsigned long pid = 0;
        std::string message;
        unsigned long win32ErrorCode = 0;
        std::string currentPreference;
    };

    struct ApplyProfileResult
    {
        bool success = false;
        int matched = 0;
        int updated = 0;
        int failed = 0;
        std::string message;
    };

    struct FrozenThreadRecord
    {
        unsigned long threadId = 0;
        unsigned long resumeCount = 0;
    };

    class ProcessActions
    {
    public:
        ProcessActionResult SetCpuPriority(unsigned long pid, const std::string& priority, bool confirmRealtime) const;
        ApplyProfileResult ApplyProfile(const Profile& profile) const;
        ProcessActionResult TerminateProcessByPid(unsigned long pid, const std::string& expectedName, const std::string& confirmation) const;
        ProcessActionResult FreezeProcessByPid(unsigned long pid, const std::string& expectedName, const std::string& confirmation);
        ProcessActionResult ResumeProcessByPid(unsigned long pid, const std::string& expectedName);
        void ResumeAllFrozenProcesses();
        bool IsFrozenByApp(unsigned long pid) const;

    private:
        static unsigned long PriorityTextToClass(const std::string& priority);
        static bool IsCriticalProcessName(const std::string& name);
        static bool EqualsIgnoreCase(std::string_view left, std::string_view right);
        static std::string FormatWin32Error(unsigned long errorCode);

        std::unordered_map<unsigned long, std::vector<FrozenThreadRecord>> m_frozenThreadsByPid;
    };
}
