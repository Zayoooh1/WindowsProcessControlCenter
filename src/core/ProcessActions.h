#pragma once

#include <string>
#include <string_view>

namespace wpcc
{
    struct ProcessActionResult
    {
        bool success = false;
        unsigned long pid = 0;
        std::string message;
        unsigned long win32ErrorCode = 0;
    };

    class ProcessActions
    {
    public:
        ProcessActionResult SetCpuPriority(unsigned long pid, const std::string& priority, bool confirmRealtime) const;
        ProcessActionResult TerminateProcessByPid(unsigned long pid, const std::string& expectedName, const std::string& confirmation) const;

    private:
        static unsigned long PriorityTextToClass(const std::string& priority);
        static bool IsCriticalProcessName(const std::string& name);
        static bool EqualsIgnoreCase(std::string_view left, std::string_view right);
        static std::string FormatWin32Error(unsigned long errorCode);
    };
}
