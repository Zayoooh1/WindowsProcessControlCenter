#include "ui_web/WebMessageBridge.h"

#include <Windows.h>

#include <cwchar>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace wpcc
{
    WebMessageType WebMessageBridge::ParseMessageType(std::wstring_view messageJson) const
    {
        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"refreshProcesses\"") != std::wstring_view::npos)
        {
            return WebMessageType::RefreshProcesses;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"getSystemMetrics\"") != std::wstring_view::npos)
        {
            return WebMessageType::GetSystemMetrics;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"getProcessDetails\"") != std::wstring_view::npos)
        {
            return WebMessageType::GetProcessDetails;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"setCpuPriority\"") != std::wstring_view::npos)
        {
            return WebMessageType::SetCpuPriority;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"setCpuAffinity\"") != std::wstring_view::npos)
        {
            return WebMessageType::SetCpuAffinity;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"terminateProcess\"") != std::wstring_view::npos)
        {
            return WebMessageType::TerminateProcess;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"freezeProcess\"") != std::wstring_view::npos)
        {
            return WebMessageType::FreezeProcess;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"resumeProcess\"") != std::wstring_view::npos)
        {
            return WebMessageType::ResumeProcess;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"setGpuPreference\"") != std::wstring_view::npos)
        {
            return WebMessageType::SetGpuPreference;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"getSettings\"") != std::wstring_view::npos)
        {
            return WebMessageType::GetSettings;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"saveSettings\"") != std::wstring_view::npos)
        {
            return WebMessageType::SaveSettings;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"getProfiles\"") != std::wstring_view::npos)
        {
            return WebMessageType::GetProfiles;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"saveProfiles\"") != std::wstring_view::npos)
        {
            return WebMessageType::SaveProfiles;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"getAutoruns\"") != std::wstring_view::npos)
        {
            return WebMessageType::GetAutoruns;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"setAutorunEnabled\"") != std::wstring_view::npos)
        {
            return WebMessageType::SetAutorunEnabled;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"exportProfilesToFile\"") != std::wstring_view::npos)
        {
            return WebMessageType::ExportProfilesToFile;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"chooseExecutable\"") != std::wstring_view::npos)
        {
            return WebMessageType::ChooseExecutable;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"applyProfile\"") != std::wstring_view::npos)
        {
            return WebMessageType::ApplyProfile;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"executeInstaller\"") != std::wstring_view::npos)
        {
            return WebMessageType::ExecuteInstaller;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"downloadUpdate\"") != std::wstring_view::npos)
        {
            return WebMessageType::DownloadUpdate;
        }

        if (messageJson.find(L"\"type\"") != std::wstring_view::npos &&
            messageJson.find(L"\"OpenExternalUrl\"") != std::wstring_view::npos)
        {
            return WebMessageType::OpenExternalUrl;
        }

        return WebMessageType::Unknown;
    }

    SetCpuPriorityRequest WebMessageBridge::ParseSetCpuPriorityRequest(std::wstring_view messageJson) const
    {
        SetCpuPriorityRequest request{};
        request.pid = ExtractUnsignedLong(messageJson, L"pid");
        request.priority = ExtractString(messageJson, L"priority");
        request.confirmRealtime = ExtractBool(messageJson, L"confirmRealtime");
        return request;
    }

    SetCpuAffinityRequest WebMessageBridge::ParseSetCpuAffinityRequest(std::wstring_view messageJson) const
    {
        SetCpuAffinityRequest request{};
        request.pid = ExtractUnsignedLong(messageJson, L"pid");
        request.validMask = TryParseCpuAffinityMask(messageJson, request.affinityMask);
        request.applyToFamily = ExtractBool(messageJson, L"applyToFamily");
        return request;
    }

    TerminateProcessRequest WebMessageBridge::ParseTerminateProcessRequest(std::wstring_view messageJson) const
    {
        TerminateProcessRequest request{};
        request.pid = ExtractUnsignedLong(messageJson, L"pid");
        request.expectedName = ExtractString(messageJson, L"expectedName");
        request.confirmation = ExtractString(messageJson, L"confirmation");
        return request;
    }

    FreezeProcessRequest WebMessageBridge::ParseFreezeProcessRequest(std::wstring_view messageJson) const
    {
        FreezeProcessRequest request{};
        request.pid = ExtractUnsignedLong(messageJson, L"pid");
        request.expectedName = ExtractString(messageJson, L"expectedName");
        request.confirmation = ExtractString(messageJson, L"confirmation");
        return request;
    }

    ResumeProcessRequest WebMessageBridge::ParseResumeProcessRequest(std::wstring_view messageJson) const
    {
        ResumeProcessRequest request{};
        request.pid = ExtractUnsignedLong(messageJson, L"pid");
        request.expectedName = ExtractString(messageJson, L"expectedName");
        return request;
    }

    std::string WebMessageBridge::ParseSaveProfilesRequest(std::wstring_view messageJson) const
    {
        return ExtractString(messageJson, L"profiles");
    }

    std::string WebMessageBridge::ParseSaveSettingsRequest(std::wstring_view messageJson) const
    {
        return ExtractString(messageJson, L"settings");
    }

    std::string WebMessageBridge::ParseApplyProfileRequest(std::wstring_view messageJson) const
    {
        return ExtractString(messageJson, L"profileId");
    }

    std::wstring WebMessageBridge::ParseExecuteInstallerRequest(std::wstring_view messageJson) const
    {
        return Utf8ToWide(ExtractString(messageJson, L"filePath"));
    }

    std::wstring WebMessageBridge::ParseDownloadUpdateUrl(std::wstring_view messageJson) const
    {
        return Utf8ToWide(ExtractString(messageJson, L"url"));
    }

    SetGpuPreferenceRequest WebMessageBridge::ParseSetGpuPreferenceRequest(std::wstring_view messageJson) const
    {
        SetGpuPreferenceRequest request{};
        request.pid = ExtractUnsignedLong(messageJson, L"pid");
        request.expectedName = ExtractString(messageJson, L"expectedName");
        request.executablePath = ExtractString(messageJson, L"exePath");
        request.preference = ExtractString(messageJson, L"preference");
        return request;
    }

    SetAutorunEnabledRequest WebMessageBridge::ParseSetAutorunEnabledRequest(std::wstring_view messageJson) const
    {
        SetAutorunEnabledRequest request{};
        request.id = ExtractString(messageJson, L"id");

        const size_t keyPosition = messageJson.find(L"\"enabled\"");
        if (keyPosition == std::wstring_view::npos)
        {
            return request;
        }
        const size_t colonPosition = messageJson.find(L':', keyPosition + std::wstring_view(L"\"enabled\"").size());
        if (colonPosition == std::wstring_view::npos)
        {
            return request;
        }
        const size_t valuePosition = messageJson.find_first_not_of(L" \t\r\n", colonPosition + 1);
        if (valuePosition == std::wstring_view::npos)
        {
            return request;
        }

        if (messageJson.substr(valuePosition, 4) == L"true")
        {
            request.enabled = true;
            request.valid = !request.id.empty();
        }
        else if (messageJson.substr(valuePosition, 5) == L"false")
        {
            request.enabled = false;
            request.valid = !request.id.empty();
        }
        return request;
    }

    unsigned long WebMessageBridge::ParseProcessDetailsRequest(std::wstring_view messageJson) const
    {
        return ExtractUnsignedLong(messageJson, L"pid");
    }

    OpenExternalUrlRequest WebMessageBridge::ParseOpenExternalUrlRequest(std::wstring_view messageJson) const
    {
        OpenExternalUrlRequest request{};
        request.url = ExtractString(messageJson, L"url");
        return request;
    }

    std::wstring WebMessageBridge::BuildProcessSnapshotMessage(const std::vector<ProcessInfo>& processes, const std::vector<AutoApplyLog>& autoApplyLogs) const
    {
        std::wostringstream json;
        json << L"{\"type\":\"processSnapshot\",\"processes\":[";

        for (size_t index = 0; index < processes.size(); ++index)
        {
            const ProcessInfo& process = processes[index];
            if (index > 0)
            {
                json << L",";
            }

            json << L"{";
            json << L"\"pid\":" << process.pid << L",";
            json << L"\"name\":\"" << EscapeJson(process.name) << L"\",";
            json << L"\"path\":\"" << EscapeJson(process.executablePath) << L"\",";
            json << L"\"cpuPriority\":\"" << EscapeJson(process.cpuPriority) << L"\",";
            json << L"\"gpuPreference\":\"" << EscapeJson(process.gpuPreference) << L"\",";
            json << L"\"isFrozenByApp\":" << (process.isFrozenByApp ? L"true" : L"false") << L",";
            json << L"\"adminNeeded\":" << (process.likelyRequiresAdmin ? L"true" : L"false") << L",";
            json << L"\"accessStatus\":\"" << EscapeJson(process.accessStatus) << L"\",";
            json << L"\"accessError\":\"" << EscapeJson(process.accessError) << L"\"";
            json << L"}";
        }

        json << L"],\"autoApplyLogs\":[";
        for (size_t index = 0; index < autoApplyLogs.size(); ++index)
        {
            const AutoApplyLog& log = autoApplyLogs[index];
            if (index > 0)
            {
                json << L",";
            }

            json << L"{";
            json << L"\"timestamp\":\"" << EscapeJson(log.timestamp) << L"\",";
            json << L"\"processName\":\"" << EscapeJson(log.processName) << L"\",";
            json << L"\"action\":\"" << EscapeJson(log.action) << L"\",";
            json << L"\"status\":\"" << EscapeJson(log.status) << L"\"";
            json << L"}";
        }

        json << L"]}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildProcessDetailsMessage(const ProcessInfo& process) const
    {
        std::wostringstream json;
        json << L"{\"type\":\"processDetails\",\"pid\":" << process.pid << L",\"details\":{";
        json << L"\"name\":\"" << EscapeJson(process.name) << L"\",";
        json << L"\"path\":\"" << EscapeJson(process.executablePath) << L"\",";
        json << L"\"cpuPriority\":\"" << EscapeJson(process.cpuPriority) << L"\",";
        json << L"\"cpuAffinityMask\":\"" << process.cpuAffinityMask << L"\",";
        json << L"\"systemAffinityMask\":\"" << process.systemAffinityMask << L"\",";
        json << L"\"cpuAffinityKnown\":" << (process.cpuAffinityKnown ? L"true" : L"false") << L",";
        json << L"\"performanceCoreMask\":\"" << process.performanceCoreMask << L"\",";
        json << L"\"performanceCoreMaskKnown\":" << (process.performanceCoreMaskKnown ? L"true" : L"false") << L",";
        json << L"\"gpuPreference\":\"" << EscapeJson(process.gpuPreference) << L"\",";
        json << L"\"isFrozenByApp\":" << (process.isFrozenByApp ? L"true" : L"false") << L",";
        json << L"\"adminNeeded\":" << (process.likelyRequiresAdmin ? L"true" : L"false") << L",";
        json << L"\"accessStatus\":\"" << EscapeJson(process.accessStatus) << L"\",";
        json << L"\"accessError\":\"" << EscapeJson(process.accessError) << L"\"}}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildSystemMetricsMessage(
        bool cpuUsageKnown,
        double cpuUsagePercent,
        bool memoryUsageKnown,
        double memoryUsagePercent,
        unsigned long long memoryUsedBytes,
        unsigned long long memoryTotalBytes) const
    {
        std::wostringstream json;
        json.imbue(std::locale::classic());
        json << L"{\"type\":\"systemMetrics\",";
        json << L"\"cpuUsageKnown\":" << (cpuUsageKnown ? L"true" : L"false") << L",";
        json << L"\"cpuUsagePercent\":";
        if (cpuUsageKnown)
        {
            json << std::fixed << std::setprecision(1) << cpuUsagePercent;
        }
        else
        {
            json << L"null";
        }

        json << L",\"memoryUsageKnown\":" << (memoryUsageKnown ? L"true" : L"false") << L",";
        json << L"\"memoryUsagePercent\":";
        if (memoryUsageKnown)
        {
            json << std::fixed << std::setprecision(1) << memoryUsagePercent;
            json << L",\"memoryUsedBytes\":\"" << memoryUsedBytes << L"\"";
            json << L",\"memoryTotalBytes\":\"" << memoryTotalBytes << L"\"";
        }
        else
        {
            json << L"null,\"memoryUsedBytes\":null,\"memoryTotalBytes\":null";
        }

        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildDownloadCompleteMessage(bool success, std::wstring_view filePath, std::wstring_view errorMessage) const
    {
        std::wostringstream json;
        json << L"{";
        json << L"\"type\":\"downloadComplete\",";
        json << L"\"success\":" << (success ? L"true" : L"false");
        if (success)
        {
            json << L",\"filePath\":\"" << EscapeJson(WideToUtf8(filePath)) << L"\"";
        }
        else
        {
            json << L",\"message\":\"" << EscapeJson(WideToUtf8(errorMessage)) << L"\"";
        }
        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildDownloadProgressMessage(uint32_t downloadedBytes, uint32_t totalBytes) const
    {
        std::wostringstream json;
        json << L"{";
        json << L"\"type\":\"downloadProgress\",";
        json << L"\"downloadedBytes\":" << downloadedBytes << L",";
        json << L"\"totalBytes\":" << totalBytes;
        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildProfilesLoadedMessage(bool success, const std::string& profilesJson, std::wstring_view warning) const
    {
        std::wostringstream json;
        json << L"{";
        json << L"\"type\":\"profilesLoaded\",";
        json << L"\"success\":" << (success ? L"true" : L"false");

        if (success && !profilesJson.empty())
        {
            json << L",\"profiles\":" << Utf8ToWide(profilesJson);
        }

        if (!warning.empty())
        {
            json << L",\"warning\":\"" << EscapeJson(WideToUtf8(warning)) << L"\"";
        }

        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildSettingsLoadedMessage(bool success, const std::string& settingsJson, std::wstring_view warning) const
    {
        std::wostringstream json;
        json << L"{";
        json << L"\"type\":\"settingsLoaded\",";
        json << L"\"success\":" << (success ? L"true" : L"false");

        if (success && !settingsJson.empty())
        {
            json << L",\"settings\":" << Utf8ToWide(settingsJson);
        }

        if (!warning.empty())
        {
            json << L",\"warning\":\"" << EscapeJson(WideToUtf8(warning)) << L"\"";
        }

        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildSettingsSavedMessage(bool success, std::wstring_view warning) const
    {
        std::wostringstream json;
        json << L"{";
        json << L"\"type\":\"settingsSaved\",";
        json << L"\"success\":" << (success ? L"true" : L"false");

        if (!success && !warning.empty())
        {
            json << L",\"warning\":\"" << EscapeJson(WideToUtf8(warning)) << L"\"";
        }

        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildProfilesSavedMessage(bool success, std::wstring_view warning) const
    {
        std::wostringstream json;
        json << L"{";
        json << L"\"type\":\"profilesSaved\",";
        json << L"\"success\":" << (success ? L"true" : L"false");

        if (!warning.empty())
        {
            json << L",\"warning\":\"" << EscapeJson(WideToUtf8(warning)) << L"\"";
        }

        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildAutorunsSnapshotMessage(const AutorunScanResult& result) const
    {
        const auto categoryName = [](AutorunCategory category) -> std::wstring_view {
            switch (category)
            {
            case AutorunCategory::ScheduledTask: return L"scheduledTask";
            case AutorunCategory::Service: return L"service";
            case AutorunCategory::Driver: return L"driver";
            default: return L"logon";
            }
        };
        const auto sourceTypeName = [](AutorunSourceType sourceType) -> std::wstring_view {
            switch (sourceType)
            {
            case AutorunSourceType::StartupFolder: return L"startupFolder";
            case AutorunSourceType::ScheduledTask: return L"scheduledTask";
            case AutorunSourceType::Service: return L"service";
            case AutorunSourceType::Driver: return L"driver";
            default: return L"registryValue";
            }
        };
        std::wostringstream json;
        json << L"{\"type\":\"autorunsSnapshot\",\"entries\":[";
        for (size_t index = 0; index < result.entries.size(); ++index)
        {
            const AutorunEntry& entry = result.entries[index];
            if (index > 0)
            {
                json << L",";
            }
            json << L"{";
            json << L"\"id\":\"" << EscapeJson(entry.id) << L"\",";
            json << L"\"category\":\"" << categoryName(entry.category) << L"\",";
            json << L"\"sourceType\":\"" << sourceTypeName(entry.sourceType) << L"\",";
            json << L"\"entryName\":\"" << EscapeJson(WideToUtf8(entry.entryName)) << L"\",";
            json << L"\"publisher\":\"\\u2014\",";
            json << L"\"command\":\"" << EscapeJson(WideToUtf8(entry.command)) << L"\",";
            json << L"\"imagePath\":\"" << EscapeJson(WideToUtf8(entry.imagePath)) << L"\",";
            json << L"\"location\":\"" << EscapeJson(WideToUtf8(entry.location)) << L"\",";
            json << L"\"user\":\"" << EscapeJson(WideToUtf8(entry.user)) << L"\",";
            json << L"\"status\":\"" << EscapeJson(WideToUtf8(entry.status)) << L"\",";
            json << L"\"enabled\":" << (entry.enabled ? L"true" : L"false") << L",";
            json << L"\"enabledKnown\":" << (entry.enabledKnown ? L"true" : L"false") << L",";
            json << L"\"canSetEnabled\":" << (entry.canSetEnabled ? L"true" : L"false") << L",";
            json << L"\"readOnlyReason\":\"" << EscapeJson(WideToUtf8(entry.readOnlyReason)) << L"\",";
            json << L"\"requiresElevation\":" << (entry.requiresElevation ? L"true" : L"false");
            json << L"}";
        }
        json << L"]";
        if (!result.warning.empty())
        {
            json << L",\"warning\":\"" << EscapeJson(WideToUtf8(result.warning)) << L"\"";
        }
        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildAutorunActionResultMessage(const AutorunActionResult& result) const
    {
        std::wostringstream json;
        json << L"{";
        json << L"\"type\":\"autorunActionResult\",";
        json << L"\"success\":" << (result.success ? L"true" : L"false") << L",";
        json << L"\"id\":\"" << EscapeJson(result.id) << L"\",";
        json << L"\"enabled\":" << (result.enabled ? L"true" : L"false") << L",";
        json << L"\"message\":\"" << EscapeJson(result.message) << L"\"";
        if (result.win32ErrorCode != 0)
        {
            json << L",\"win32ErrorCode\":" << result.win32ErrorCode;
        }
        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildProfilesExportedMessage(bool success, bool cancelled, std::wstring_view warning) const
    {
        std::wostringstream json;
        json << L"{";
        json << L"\"type\":\"profilesExported\",";
        json << L"\"success\":" << (success ? L"true" : L"false") << L",";
        json << L"\"cancelled\":" << (cancelled ? L"true" : L"false");

        if (!warning.empty())
        {
            json << L",\"warning\":\"" << EscapeJson(WideToUtf8(warning)) << L"\"";
        }

        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildExecutableChosenMessage(bool success, bool cancelled, std::wstring_view path, std::wstring_view fileName, std::string_view iconDataUrl) const
    {
        std::wostringstream json;
        json << L"{";
        json << L"\"type\":\"executableChosen\",";
        json << L"\"success\":" << (success ? L"true" : L"false") << L",";
        json << L"\"cancelled\":" << (cancelled ? L"true" : L"false");

        if (success && !path.empty())
        {
            json << L",\"path\":\"" << EscapeJson(WideToUtf8(path)) << L"\"";
            json << L",\"fileName\":\"" << EscapeJson(WideToUtf8(fileName)) << L"\"";
            if (!iconDataUrl.empty())
            {
                json << L",\"iconDataUrl\":\"" << EscapeJson(iconDataUrl) << L"\"";
            }
        }

        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildProfileAppliedMessage(const std::string& profileId, bool success, int matched, int updated, int failed, std::string_view message) const
    {
        std::wostringstream json;
        json << L"{";
        json << L"\"type\":\"profileApplied\",";
        json << L"\"profileId\":\"" << EscapeJson(profileId) << L"\",";
        json << L"\"success\":" << (success ? L"true" : L"false") << L",";
        json << L"\"matched\":" << matched << L",";
        json << L"\"updated\":" << updated << L",";
        json << L"\"failed\":" << failed << L",";
        json << L"\"message\":\"" << EscapeJson(message) << L"\"";
        json << L"}";
        return json.str();
    }

    std::wstring WebMessageBridge::BuildErrorMessage(std::string_view message) const
    {
        return L"{\"type\":\"error\",\"message\":\"" + EscapeJson(message) + L"\"}";
    }

    std::wstring WebMessageBridge::BuildActionResultMessage(std::string_view action, const ProcessActionResult& result) const
    {
        std::wostringstream json;
        json << L"{";
        json << L"\"type\":\"actionResult\",";
        json << L"\"action\":\"" << EscapeJson(action) << L"\",";
        json << L"\"success\":" << (result.success ? L"true" : L"false") << L",";
        json << L"\"message\":\"" << EscapeJson(result.message) << L"\",";
        json << L"\"pid\":" << result.pid;
        if (result.win32ErrorCode != 0)
        {
            json << L",\"win32ErrorCode\":" << result.win32ErrorCode;
        }
        if (!result.currentPreference.empty())
        {
            json << L",\"currentPreference\":\"" << EscapeJson(result.currentPreference) << L"\"";
        }
        json << L"}";
        return json.str();
    }

    unsigned long WebMessageBridge::ExtractUnsignedLong(std::wstring_view json, std::wstring_view key)
    {
        const std::wstring pattern = L"\"" + std::wstring(key) + L"\"";
        const size_t keyPos = json.find(pattern);
        if (keyPos == std::wstring_view::npos)
        {
            return 0;
        }

        const size_t colonPos = json.find(L":", keyPos + pattern.size());
        if (colonPos == std::wstring_view::npos)
        {
            return 0;
        }

        size_t valueStart = json.find_first_of(L"0123456789", colonPos + 1);
        if (valueStart == std::wstring_view::npos)
        {
            return 0;
        }

        size_t valueEnd = valueStart;
        while (valueEnd < json.size() && json[valueEnd] >= L'0' && json[valueEnd] <= L'9')
        {
            ++valueEnd;
        }

        const std::wstring value(json.substr(valueStart, valueEnd - valueStart));
        wchar_t* end = nullptr;
        return std::wcstoul(value.c_str(), &end, 10);
    }

    bool WebMessageBridge::TryParseCpuAffinityMask(std::wstring_view json, unsigned long long& affinityMask)
    {
        const size_t keyPos = json.find(L"\"affinityMask\"");
        if (keyPos == std::wstring_view::npos)
        {
            return false;
        }

        const size_t colonPos = json.find(L":", keyPos + std::wstring_view(L"\"affinityMask\"").size());
        if (colonPos == std::wstring_view::npos)
        {
            return false;
        }

        const size_t valueStart = json.find_first_not_of(L" \t\r\n", colonPos + 1);
        if (valueStart == std::wstring_view::npos || json[valueStart] != L'\"')
        {
            return false;
        }

        unsigned long long parsedMask = 0;
        bool hasDigit = false;
        for (size_t index = valueStart + 1; index < json.size(); ++index)
        {
            const wchar_t character = json[index];
            if (character == L'\"')
            {
                if (!hasDigit)
                {
                    return false;
                }

                affinityMask = parsedMask;
                return true;
            }

            if (character < L'0' || character > L'9')
            {
                return false;
            }

            const unsigned long long digit = static_cast<unsigned long long>(character - L'0');
            if (parsedMask > (std::numeric_limits<unsigned long long>::max() - digit) / 10)
            {
                return false;
            }

            parsedMask = parsedMask * 10 + digit;
            hasDigit = true;
        }

        return false;
    }

    bool WebMessageBridge::ExtractBool(std::wstring_view json, std::wstring_view key)
    {
        const std::wstring pattern = L"\"" + std::wstring(key) + L"\"";
        const size_t keyPos = json.find(pattern);
        if (keyPos == std::wstring_view::npos)
        {
            return false;
        }

        const size_t colonPos = json.find(L":", keyPos + pattern.size());
        if (colonPos == std::wstring_view::npos)
        {
            return false;
        }

        const size_t valueStart = json.find_first_not_of(L" \t\r\n", colonPos + 1);
        return valueStart != std::wstring_view::npos && json.substr(valueStart, 4) == L"true";
    }

    std::string WebMessageBridge::ExtractString(std::wstring_view json, std::wstring_view key)
    {
        const std::wstring pattern = L"\"" + std::wstring(key) + L"\"";
        const size_t keyPos = json.find(pattern);
        if (keyPos == std::wstring_view::npos)
        {
            return {};
        }

        const size_t colonPos = json.find(L":", keyPos + pattern.size());
        if (colonPos == std::wstring_view::npos)
        {
            return {};
        }

        const size_t quoteStart = json.find(L"\"", colonPos + 1);
        if (quoteStart == std::wstring_view::npos)
        {
            return {};
        }

        std::wstring value;
        for (size_t index = quoteStart + 1; index < json.size(); ++index)
        {
            const wchar_t character = json[index];
            if (character == L'\"')
            {
                break;
            }

            if (character == L'\\' && index + 1 < json.size())
            {
                const wchar_t escaped = json[++index];
                switch (escaped)
                {
                case L'\"':
                case L'\\':
                case L'/':
                    value.push_back(escaped);
                    break;
                case L'b':
                    value.push_back(L'\b');
                    break;
                case L'f':
                    value.push_back(L'\f');
                    break;
                case L'n':
                    value.push_back(L'\n');
                    break;
                case L'r':
                    value.push_back(L'\r');
                    break;
                case L't':
                    value.push_back(L'\t');
                    break;
                default:
                    value.push_back(escaped);
                    break;
                }
            }
            else
            {
                value.push_back(character);
            }
        }

        return WideToUtf8(value);
    }

    std::wstring WebMessageBridge::EscapeJson(std::string_view value)
    {
        std::wstring wide = Utf8ToWide(value);
        std::wostringstream escaped;

        for (wchar_t character : wide)
        {
            switch (character)
            {
            case L'\"':
                escaped << L"\\\"";
                break;
            case L'\\':
                escaped << L"\\\\";
                break;
            case L'\b':
                escaped << L"\\b";
                break;
            case L'\f':
                escaped << L"\\f";
                break;
            case L'\n':
                escaped << L"\\n";
                break;
            case L'\r':
                escaped << L"\\r";
                break;
            case L'\t':
                escaped << L"\\t";
                break;
            default:
                if (character < 0x20)
                {
                    escaped << L"\\u";
                    constexpr wchar_t hex[] = L"0123456789abcdef";
                    escaped << hex[(character >> 12) & 0xF];
                    escaped << hex[(character >> 8) & 0xF];
                    escaped << hex[(character >> 4) & 0xF];
                    escaped << hex[character & 0xF];
                }
                else
                {
                    escaped << character;
                }
                break;
            }
        }

        return escaped.str();
    }

    std::wstring WebMessageBridge::Utf8ToWide(std::string_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int requiredSize = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (requiredSize <= 0)
        {
            return {};
        }

        std::wstring result(static_cast<size_t>(requiredSize), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), requiredSize);
        return result;
    }

    std::string WebMessageBridge::WideToUtf8(std::wstring_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int requiredSize = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (requiredSize <= 0)
        {
            return {};
        }

        std::string result(static_cast<size_t>(requiredSize), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), requiredSize, nullptr, nullptr);
        return result;
    }
}
