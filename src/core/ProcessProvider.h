#pragma once

#include "core/ProcessInfo.h"

#include <vector>

namespace wpcc
{
    class ProcessProvider
    {
    public:
        std::vector<ProcessInfo> LoadProcesses() const;
    };
}
