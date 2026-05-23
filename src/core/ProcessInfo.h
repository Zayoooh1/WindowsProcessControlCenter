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
        std::string accessStatus;
        std::string accessError;
        bool likelyRequiresAdmin = false;
        bool isFrozenByApp = false;
    };
}
