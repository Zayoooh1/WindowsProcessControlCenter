#pragma once

#include "core/ProcessActions.h"
#include "core/ProcessInfo.h"

#include <string>
#include <string_view>
#include <vector>

namespace wpcc
{
    enum class WebMessageType
    {
        Unknown,
        RefreshProcesses,
        SetCpuPriority,
        TerminateProcess,
        FreezeProcess,
        ResumeProcess,
        SetGpuPreference,
        LoadProfiles,
        SaveProfiles,
        ExportProfilesToFile,
        ChooseExecutable,
        ApplyProfile,
        ExecuteInstaller,
        DownloadUpdate,
    };

    struct SetCpuPriorityRequest
    {
        unsigned long pid = 0;
        std::string priority;
        bool confirmRealtime = false;
    };

    struct TerminateProcessRequest
    {
        unsigned long pid = 0;
        std::string expectedName;
        std::string confirmation;
    };

    struct FreezeProcessRequest
    {
        unsigned long pid = 0;
        std::string expectedName;
        std::string confirmation;
    };

    struct ResumeProcessRequest
    {
        unsigned long pid = 0;
        std::string expectedName;
    };

    struct SetGpuPreferenceRequest
    {
        unsigned long pid = 0;
        std::string expectedName;
        std::string executablePath;
        std::string preference;
    };

    class WebMessageBridge
    {
    public:
        WebMessageType ParseMessageType(std::wstring_view messageJson) const;
        SetCpuPriorityRequest ParseSetCpuPriorityRequest(std::wstring_view messageJson) const;
        TerminateProcessRequest ParseTerminateProcessRequest(std::wstring_view messageJson) const;
        FreezeProcessRequest ParseFreezeProcessRequest(std::wstring_view messageJson) const;
        ResumeProcessRequest ParseResumeProcessRequest(std::wstring_view messageJson) const;
        SetGpuPreferenceRequest ParseSetGpuPreferenceRequest(std::wstring_view messageJson) const;
        std::string ParseSaveProfilesRequest(std::wstring_view messageJson) const;
        std::string ParseApplyProfileRequest(std::wstring_view messageJson) const;
        std::wstring ParseExecuteInstallerRequest(std::wstring_view messageJson) const;
        std::wstring ParseDownloadUpdateUrl(std::wstring_view messageJson) const;
        std::wstring BuildProcessSnapshotMessage(const std::vector<ProcessInfo>& processes) const;
        std::wstring BuildDownloadCompleteMessage(bool success, std::wstring_view filePath, std::wstring_view errorMessage) const;
        std::wstring BuildDownloadProgressMessage(uint32_t downloadedBytes, uint32_t totalBytes) const;
        std::wstring BuildActionResultMessage(std::string_view action, const ProcessActionResult& result) const;
        std::wstring BuildProfilesLoadedMessage(bool success, const std::string& profilesJson, std::wstring_view warning) const;
        std::wstring BuildProfilesSavedMessage(bool success, std::wstring_view warning) const;
        std::wstring BuildProfilesExportedMessage(bool success, bool cancelled, std::wstring_view warning) const;
        std::wstring BuildExecutableChosenMessage(bool success, bool cancelled, std::wstring_view path, std::wstring_view fileName, std::string_view iconDataUrl) const;
        std::wstring BuildProfileAppliedMessage(const std::string& profileId, bool success, int matched, int updated, int failed, std::string_view message) const;
        std::wstring BuildErrorMessage(std::string_view message) const;

    private:
        static unsigned long ExtractUnsignedLong(std::wstring_view json, std::wstring_view key);
        static bool ExtractBool(std::wstring_view json, std::wstring_view key);
        static std::string ExtractString(std::wstring_view json, std::wstring_view key);
        static std::wstring EscapeJson(std::string_view value);
        static std::wstring Utf8ToWide(std::string_view value);
        static std::string WideToUtf8(std::wstring_view value);
    };
}
