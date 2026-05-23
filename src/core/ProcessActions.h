#pragma once

#include <string>

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

    private:
        static unsigned long PriorityTextToClass(const std::string& priority);
        static std::string FormatWin32Error(unsigned long errorCode);
    };
}
