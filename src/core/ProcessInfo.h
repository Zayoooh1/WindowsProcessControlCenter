#pragma once

#include <string>

namespace wpcc
{
    struct ProcessInfo
    {
        unsigned long pid = 0;
        std::string name;
        std::string executablePath;
        std::string cpuPriority;
        unsigned long long cpuAffinityMask = 0;
        unsigned long long systemAffinityMask = 0;
        bool cpuAffinityKnown = false;
        unsigned long long performanceCoreMask = 0;
        bool performanceCoreMaskKnown = false;
        std::string gpuPreference = "Unknown";
        std::string accessStatus;
        std::string accessError;
        bool likelyRequiresAdmin = false;
        bool isFrozenByApp = false;
    };
}
