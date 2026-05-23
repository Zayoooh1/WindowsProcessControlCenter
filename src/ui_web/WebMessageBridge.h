#pragma once

#include "core/ProcessActions.h"
#include "core/ProcessInfo.h"

#include <string>
#include <string_view>
#include <vector>

namespace wpcc
{
    enum class WebMessageType
    {
        Unknown,
        RefreshProcesses,
        SetCpuPriority,
    };

    struct SetCpuPriorityRequest
    {
        unsigned long pid = 0;
        std::string priority;
        bool confirmRealtime = false;
    };

    class WebMessageBridge
    {
    public:
        WebMessageType ParseMessageType(std::wstring_view messageJson) const;
        SetCpuPriorityRequest ParseSetCpuPriorityRequest(std::wstring_view messageJson) const;
        std::wstring BuildProcessSnapshotMessage(const std::vector<ProcessInfo>& processes) const;
        std::wstring BuildActionResultMessage(std::string_view action, const ProcessActionResult& result) const;
        std::wstring BuildErrorMessage(std::string_view message) const;

    private:
        static unsigned long ExtractUnsignedLong(std::wstring_view json, std::wstring_view key);
        static bool ExtractBool(std::wstring_view json, std::wstring_view key);
        static std::string ExtractString(std::wstring_view json, std::wstring_view key);
        static std::wstring EscapeJson(std::string_view value);
        static std::wstring Utf8ToWide(std::string_view value);
        static std::string WideToUtf8(std::wstring_view value);
    };
}
