#include "core/ProfileStore.h"

#include <Windows.h>
#include <ShlObj.h>

#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <iterator>

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

        // Fallback to environment variable
        const char* appData = std::getenv("APPDATA");
        if (appData)
        {
            std::filesystem::path path(appData);
            path /= "WindowsProcessControlCenter";
            return path / "profiles.json";
        }

        return {};
    }

    bool ProfileStore::IsValidJson(std::string_view json)
    {
        // Simple sanity check for valid JSON structure:
        // Trim whitespace and check that it starts with '{' and ends with '}'
        if (json.empty())
        {
            return false;
        }

        size_t first = json.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos || json[first] != '{')
        {
            return false;
        }

        size_t last = json.find_last_not_of(" \t\r\n");
        if (last == std::string_view::npos || json[last] != '}')
        {
            return false;
        }

        // Verify basic balanced quotes and bracket counts to ensure it's not totally broken
        size_t openBraces = 0;
        size_t closeBraces = 0;
        bool inString = false;
        bool escaped = false;

        for (char character : json)
        {
            if (inString)
            {
                if (escaped)
                {
                    escaped = false;
                }
                else if (character == '\\')
                {
                    escaped = true;
                }
                else if (character == '"')
                {
                    inString = false;
                }
            }
            else
            {
                if (character == '"')
                {
                    inString = true;
                }
                else if (character == '{')
                {
                    openBraces++;
                }
                else if (character == '}')
                {
                    closeBraces++;
                }
            }
        }

        return !inString && openBraces == closeBraces && openBraces > 0;
    }

    int ProfileStore::ExtractSchemaVersion(std::string_view json)
    {
        size_t pos = json.find("\"schemaVersion\"");
        if (pos == std::string_view::npos)
        {
            return -1;
        }

        size_t colon = json.find(':', pos);
        if (colon == std::string_view::npos)
        {
            return -1;
        }

        size_t valStart = json.find_first_of("0123456789", colon + 1);
        if (valStart == std::string_view::npos)
        {
            return -1;
        }

        size_t valEnd = json.find_first_not_of("0123456789", valStart);
        std::string verStr(json.substr(valStart, valEnd - valStart));
        
        try
        {
            return std::stoi(verStr);
        }
        catch (...)
        {
            return -1;
        }
    }

    ProfileLoadResult ProfileStore::LoadProfiles()
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

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            // Validate standard JSON formatting
            if (!IsValidJson(content))
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

            // Verify version compatibility
            const int version = ExtractSchemaVersion(content);
            if (version != 1)
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

                return { false, "", L"Native profiles file used an unsupported schema version (" + std::to_wstring(version) + L"). A backup was saved and local defaults are active." };
            }

            return { true, content, L"" };
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

            file.write(profilesJson.data(), profilesJson.size());
            file.close();

            return { true, L"" };
        }
        catch (...)
        {
            return { false, L"An unexpected error occurred while saving native profiles." };
        }
    }
}
