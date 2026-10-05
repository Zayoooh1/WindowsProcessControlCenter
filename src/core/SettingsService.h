#pragma once
#include "core/SettingsStore.h"
#include "core/StartupManager.h"
namespace wpcc
{
    struct SettingsUpdateResult
    {
        bool success = false;
        std::string jsonContent;
        std::wstring warning;
        StartupState startup;
    };
    class SettingsService
    {
    public:
        static SettingsUpdateResult Load();
        static SettingsUpdateResult Save(const std::string& requested, bool changeStartup);
    };
}
