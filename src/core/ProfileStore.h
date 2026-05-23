#pragma once

#include <string>
#include <filesystem>

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

    class ProfileStore
    {
    public:
        static ProfileLoadResult LoadProfiles();
        static ProfileSaveResult SaveProfiles(const std::string& profilesJson);

    private:
        static std::filesystem::path GetProfilesFilePath();
        static bool IsValidJson(std::string_view json);
        static int ExtractSchemaVersion(std::string_view json);
    };
}
