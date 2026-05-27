#include "core/SettingsStore.h"

#include <Windows.h>
#include <ShlObj.h>

#include <fstream>
#include <cstdlib>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace wpcc
{
    std::filesystem::path SettingsStore::GetSettingsFilePath()
    {
        wchar_t appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath)))
        {
            std::filesystem::path path(appDataPath);
            path /= L"WindowsProcessControlCenter";
            return path / L"settings.json";
        }

        const char* appData = std::getenv("APPDATA");
        if (appData)
        {
            std::filesystem::path path(appData);
            path /= "WindowsProcessControlCenter";
            return path / "settings.json";
        }

        return {};
    }

    SettingsLoadResult SettingsStore::GetSettings()
    {
        const std::filesystem::path path = GetSettingsFilePath();
        if (path.empty())
        {
            return { false, "", L"Unable to determine system AppData directory." };
        }

        if (!std::filesystem::exists(path))
        {
            // Return empty JSON object string since app.js handles defaults
            return { true, "{}", L"" };
        }

        try
        {
            std::ifstream file(path, std::ios::in | std::ios::binary);
            if (!file)
            {
                return { false, "", L"Failed to open native settings file for reading." };
            }

            json j;
            file >> j;
            file.close();

            return { true, j.dump(), L"" };
        }
        catch (const json::exception&)
        {
            // Create backup of corrupted file
            try
            {
                std::filesystem::path corruptedPath = path;
                corruptedPath.replace_extension(L".json.corrupted");
                std::filesystem::copy_file(path, corruptedPath, std::filesystem::copy_options::overwrite_existing);
                std::filesystem::remove(path);
            }
            catch (...) {}

            return { false, "", L"Native settings file contained corrupted or invalid JSON. A backup was saved and local defaults are active." };
        }
        catch (...)
        {
            return { false, "", L"An unexpected error occurred while reading native settings." };
        }
    }

    SettingsSaveResult SettingsStore::SaveSettings(const std::string& settingsJson)
    {
        const std::filesystem::path path = GetSettingsFilePath();
        if (path.empty())
        {
            return { false, L"Unable to determine system AppData directory." };
        }

        try
        {
            json j = json::parse(settingsJson);

            std::filesystem::create_directories(path.parent_path());

            if (std::filesystem::exists(path))
            {
                std::filesystem::path backupPath = path;
                backupPath.replace_extension(L".json.bak");
                std::filesystem::copy_file(path, backupPath, std::filesystem::copy_options::overwrite_existing);
            }

            std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!file)
            {
                return { false, L"Failed to open native settings file for writing." };
            }

            file << j.dump(4);
            file.close();

            return { true, L"" };
        }
        catch (const json::exception&)
        {
            return { false, L"Provided settings data is not valid JSON." };
        }
        catch (...)
        {
            return { false, L"An unexpected error occurred while saving native settings." };
        }
    }

    AppSettings SettingsStore::ParseSettingsJson(const std::string& json_str)
    {
        AppSettings settings;
        
        try
        {
            json j = json::parse(json_str);
            if (j.contains("startWithWindows") && j["startWithWindows"].is_boolean())
            {
                settings.startWithWindows = j["startWithWindows"].get<bool>();
            }
            if (j.contains("minimizeToTray") && j["minimizeToTray"].is_boolean())
            {
                settings.minimizeToTray = j["minimizeToTray"].get<bool>();
            }
        }
        catch (...)
        {
            // Ignore parse errors, return defaults
        }

        return settings;
    }
}
