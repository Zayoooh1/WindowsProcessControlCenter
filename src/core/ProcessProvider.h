#pragma once

#include "core/ProcessInfo.h"

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace wpcc
{
    class ProcessProvider
    {
    public:
        std::vector<ProcessInfo> LoadProcesses() const;
        // Enumerates only PID and executable name. This avoids opening every
        // process when callers can first narrow candidates by name.
        std::vector<ProcessInfo> LoadProcessNames() const;
        ProcessInfo GetProcess(unsigned long pid) const;
        std::vector<unsigned long> GetCachedProcessIdsMatchingName(std::string_view targetName) const;
        static bool MatchesProcessName(std::string_view processName, std::string_view targetName);
        // Resolves a single process without constructing a full process list.

    private:
        mutable std::mutex m_processNameCacheMutex;
        mutable std::unordered_map<unsigned long, std::string> m_processNameCache;
    };
}
