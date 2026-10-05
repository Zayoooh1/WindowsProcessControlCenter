#include "core/SettingsStore.h"

#include <Windows.h>
#include <ShlObj.h>

#include <fstream>
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

        const DWORD requiredLength = GetEnvironmentVariableW(L"APPDATA", nullptr, 0);
        if (requiredLength > 0)
        {
            std::wstring appData(requiredLength, L'\0');
            const DWORD actualLength = GetEnvironmentVariableW(L"APPDATA", appData.data(), requiredLength);
            if (actualLength > 0 && actualLength < requiredLength)
            {
                appData.resize(actualLength);
                std::filesystem::path path(appData);
                path /= L"WindowsProcessControlCenter";
                return path / L"settings.json";
            }
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

        std::error_code ec;
        const bool exists = std::filesystem::exists(path, ec);
        if (ec) return { false, "", L"Failed to inspect native settings file; existing data was preserved." };
        if (!exists) return { true, "{\"startWithWindows\":false,\"minimizeToTray\":false}", L"" };

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
                // Preserve the original: startup reconciliation must never reset user settings.
            }
            catch (...) {}

            return { false, "", L"Native settings file contained corrupted or invalid JSON. The original file was preserved; repair it before saving." };
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

            // Write beside the target and atomically replace it only after a checked flush.
            auto temporary = path;
            temporary += L".tmp";
            const std::string bytes = j.dump(4);
            HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE) return { false, L"Failed to open temporary settings file." };
            DWORD written = 0;
            const bool writtenOk = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size();
            const bool flushed = writtenOk && FlushFileBuffers(file);
            CloseHandle(file);
            if (!flushed || !MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            {
                DeleteFileW(temporary.c_str());
                return { false, L"Failed to commit settings file; the previous settings were preserved." };
            }

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
