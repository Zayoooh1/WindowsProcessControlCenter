#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace wpcc
{
    enum class AutorunCategory
    {
        Logon,
        ScheduledTask,
        Service,
        Driver,
        Explorer,
        Winlogon,
        AppInit,
        ImageHijack,
        KnownDll,
    };

    enum class AutorunSourceType
    {
        RegistryValue,
        StartupFolder,
        ScheduledTask,
        Service,
        Driver,
    };

    struct AutorunEntry
    {
        std::string id;
        AutorunCategory category = AutorunCategory::Logon;
        AutorunSourceType sourceType = AutorunSourceType::RegistryValue;

        std::wstring entryName;
        std::wstring command;
        std::wstring imagePath;
        std::wstring location;
        std::wstring user;
        std::wstring status;

        bool enabled = true;
        bool enabledKnown = true;
        bool canSetEnabled = true;
        bool requiresElevation = false;
        std::wstring readOnlyReason;

        // Registry metadata is retained verbatim so disabling and restoring can
        // preserve the original value without interpreting its contents.
        bool registryCurrentUser = true;
        unsigned long registryView = 0;
        std::wstring registryKeyPath;
        std::wstring registryValueName;
        unsigned long registryValueType = 0;
        std::vector<unsigned char> registryValueData;

        // Startup-folder metadata identifies both sides of a reversible move.
        std::wstring startupOriginalPath;
        std::wstring startupDisabledPath;
        bool startupCurrentUser = true;

        // Task Scheduler identities are full paths (including the folder) so
        // tasks with the same leaf name remain distinct.
        std::wstring taskPath;

        // Service entries use the SCM key name as their stable identity. For
        // services disabled by WPCC, the exact prior start type is persisted.
        std::wstring serviceName;
        unsigned long serviceStartType = 0;
        unsigned long serviceOriginalStartType = 0;
    };

    struct AutorunScanResult
    {
        std::vector<AutorunEntry> entries;
        std::wstring warning;
    };

    struct AutorunActionResult
    {
        bool success = false;
        std::string id;
        bool enabled = false;
        std::string message;
        unsigned long win32ErrorCode = 0;
    };

    class AutorunProvider
    {
    public:
        AutorunScanResult GetLogonEntries();
        AutorunActionResult SetEnabled(std::string_view id, bool enabled);

    private:
        std::unordered_map<std::string, AutorunEntry> m_entries;
        std::unordered_set<std::string> m_ambiguousIds;
    };
}
