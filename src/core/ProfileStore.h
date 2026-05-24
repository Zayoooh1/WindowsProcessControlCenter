#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace wpcc
{
    struct ProfileLoadResult
    {
        bool success = false;
        std::string jsonContent;
        std::wstring warning;
    };

    struct ProfileSaveResult
    {
        bool success = false;
        std::wstring warning;
    };

    struct Profile
    {
        std::string id;
        std::string name;
        std::string targetExePath;
        std::string targetProcessName;
        std::string matchMode;
        std::string cpuPriority;
        bool allowRealtime = false;
    };

    class ProfileStore
    {
    public:
        static ProfileLoadResult LoadProfiles();
        static ProfileSaveResult SaveProfiles(const std::string& profilesJson);
        static std::vector<Profile> ParseProfilesJson(const std::string& json);

    private:
        static std::filesystem::path GetProfilesFilePath();
        static bool IsValidJson(std::string_view json);
        static int ExtractSchemaVersion(std::string_view json);
    };
}
