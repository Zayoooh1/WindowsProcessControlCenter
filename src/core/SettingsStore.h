#pragma once

#include <string>
#include <filesystem>

namespace wpcc
{
    struct SettingsLoadResult
    {
        bool success = false;
        std::string jsonContent;
        std::wstring warning;
    };

    struct SettingsSaveResult
    {
        bool success = false;
        std::wstring warning;
    };

    struct AppSettings
    {
        bool startWithWindows = false;
        bool minimizeToTray = false;
    };

    class SettingsStore
    {
    public:
        static SettingsLoadResult GetSettings();
        static SettingsSaveResult SaveSettings(const std::string& settingsJson);
        static AppSettings ParseSettingsJson(const std::string& json);

    private:
        static std::filesystem::path GetSettingsFilePath();
    };
}
