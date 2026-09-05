#pragma once

#include "core/ProcessInfo.h"

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
        // Resolves a single process without constructing a full process list.
    };
}
