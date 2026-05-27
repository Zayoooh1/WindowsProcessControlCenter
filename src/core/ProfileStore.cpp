#include "core/ProfileStore.h"

#include <Windows.h>
#include <ShlObj.h>

#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace wpcc
{
    std::filesystem::path ProfileStore::GetProfilesFilePath()
    {
        wchar_t appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath)))
        {
            std::filesystem::path path(appDataPath);
            path /= L"WindowsProcessControlCenter";
            return path / L"profiles.json";
        }

        const char* appData = std::getenv("APPDATA");
        if (appData)
        {
            std::filesystem::path path(appData);
            path /= "WindowsProcessControlCenter";
            return path / "profiles.json";
        }

        return {};
    }

    ProfileLoadResult ProfileStore::GetProfiles()
    {
        const std::filesystem::path path = GetProfilesFilePath();
        if (path.empty())
        {
            return { false, "", L"Unable to determine system AppData directory." };
        }

        if (!std::filesystem::exists(path))
        {
            // First time run or file deleted. Return empty profile payload
            return { true, "{\"schemaVersion\":1,\"profiles\":[]}", L"" };
        }

        try
        {
            std::ifstream file(path, std::ios::in | std::ios::binary);
            if (!file)
            {
                return { false, "", L"Failed to open native profiles file for reading." };
            }

            json j;
            file >> j;
            file.close();

            // Verify version compatibility
            if (!j.contains("schemaVersion") || !j["schemaVersion"].is_number_integer() || j["schemaVersion"] != 1)
            {
                // Create backup of unsupported version
                try
                {
                    std::filesystem::path corruptedPath = path;
                    corruptedPath.replace_extension(L".json.corrupted");
                    std::filesystem::copy_file(path, corruptedPath, std::filesystem::copy_options::overwrite_existing);
                    std::filesystem::remove(path);
                }
                catch (...) {}

                return { false, "", L"Native profiles file used an unsupported schema version. A backup was saved and local defaults are active." };
            }

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

            return { false, "", L"Native profiles file contained corrupted or invalid JSON. A backup was saved and local defaults are active." };
        }
        catch (...)
        {
            return { false, "", L"An unexpected error occurred while reading native profiles." };
        }
    }

    ProfileSaveResult ProfileStore::SaveProfiles(const std::string& profilesJson)
    {
        const std::filesystem::path path = GetProfilesFilePath();
        if (path.empty())
        {
            return { false, L"Unable to determine system AppData directory." };
        }

        try
        {
            // Validate JSON before writing
            json j = json::parse(profilesJson);

            // Ensure parent directory exists
            std::filesystem::create_directories(path.parent_path());

            // Create standard backup file of the existing version before writing the new content
            if (std::filesystem::exists(path))
            {
                std::filesystem::path backupPath = path;
                backupPath.replace_extension(L".json.bak");
                std::filesystem::copy_file(path, backupPath, std::filesystem::copy_options::overwrite_existing);
            }

            // Write the new JSON content
            std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!file)
            {
                return { false, L"Failed to open native profiles file for writing." };
            }

            file << j.dump(4);
            file.close();

            return { true, L"" };
        }
        catch (const json::exception&)
        {
            return { false, L"Provided profiles data is not valid JSON." };
        }
        catch (...)
        {
            return { false, L"An unexpected error occurred while saving native profiles." };
        }
    }

    std::vector<Profile> ProfileStore::ParseProfilesJson(const std::string& json_str)
    {
        std::vector<Profile> profiles;
        
        try
        {
            json j = json::parse(json_str);
            if (j.contains("profiles") && j["profiles"].is_array())
            {
                for (const auto& item : j["profiles"])
                {
                    Profile profile;
                    if (item.contains("id") && item["id"].is_string()) profile.id = item["id"].get<std::string>();
                    if (item.contains("name") && item["name"].is_string()) profile.name = item["name"].get<std::string>();
                    if (item.contains("targetExePath") && item["targetExePath"].is_string()) profile.targetExePath = item["targetExePath"].get<std::string>();
                    if (item.contains("targetProcessName") && item["targetProcessName"].is_string()) profile.targetProcessName = item["targetProcessName"].get<std::string>();
                    if (item.contains("matchMode") && item["matchMode"].is_string()) profile.matchMode = item["matchMode"].get<std::string>();
                    if (item.contains("cpuPriority") && item["cpuPriority"].is_string()) profile.cpuPriority = item["cpuPriority"].get<std::string>();
                    if (item.contains("gpuPreference") && item["gpuPreference"].is_string()) profile.gpuPreference = item["gpuPreference"].get<std::string>();
                    if (item.contains("allowRealtime") && item["allowRealtime"].is_boolean()) profile.allowRealtime = item["allowRealtime"].get<bool>();
                    if (item.contains("autoApply") && item["autoApply"].is_boolean()) profile.autoApply = item["autoApply"].get<bool>();
                    profiles.push_back(profile);
                }
            }
        }
        catch (...)
        {
            // Ignore parse errors, return what was successfully parsed
        }

        return profiles;
    }
}
