#include "core/AutorunProvider.h"

#include <Windows.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <taskschd.h>
#include <wrl/client.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cwctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <system_error>
#include <utility>

using json = nlohmann::json;
using Microsoft::WRL::ComPtr;

namespace
{
    constexpr wchar_t RunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    constexpr wchar_t RunOnceKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
    constexpr int DisabledStoreSchemaVersion = 1;

    class RegistryKey final
    {
    public:
        RegistryKey() = default;
        RegistryKey(const RegistryKey&) = delete;
        RegistryKey& operator=(const RegistryKey&) = delete;

        ~RegistryKey()
        {
            if (m_key != nullptr)
            {
                RegCloseKey(m_key);
            }
        }

        HKEY* Put()
        {
            return &m_key;
        }

        HKEY Get() const
        {
            return m_key;
        }

    private:
        HKEY m_key = nullptr;
    };

    class FindFileHandle final
    {
    public:
        explicit FindFileHandle(HANDLE handle) : m_handle(handle) {}
        FindFileHandle(const FindFileHandle&) = delete;
        FindFileHandle& operator=(const FindFileHandle&) = delete;

        ~FindFileHandle()
        {
            if (m_handle != INVALID_HANDLE_VALUE)
            {
                FindClose(m_handle);
            }
        }

        HANDLE Get() const
        {
            return m_handle;
        }

    private:
        HANDLE m_handle = INVALID_HANDLE_VALUE;
    };

    class ServiceHandle final
    {
    public:
        ServiceHandle() = default;
        explicit ServiceHandle(SC_HANDLE handle) : m_handle(handle) {}
        ServiceHandle(const ServiceHandle&) = delete;
        ServiceHandle& operator=(const ServiceHandle&) = delete;
        ~ServiceHandle() { if (m_handle != nullptr) CloseServiceHandle(m_handle); }
        SC_HANDLE Get() const { return m_handle; }
    private:
        SC_HANDLE m_handle = nullptr;
    };

    class ScopedBstr final
    {
    public:
        ScopedBstr() = default;
        explicit ScopedBstr(std::wstring_view value)
            : m_value(SysAllocStringLen(value.data(), static_cast<UINT>(value.size()))) {}
        ScopedBstr(const ScopedBstr&) = delete;
        ScopedBstr& operator=(const ScopedBstr&) = delete;
        ~ScopedBstr() { SysFreeString(m_value); }
        BSTR Get() const { return m_value; }
        BSTR* Put() { SysFreeString(m_value); m_value = nullptr; return &m_value; }
        std::wstring String() const
        {
            return m_value == nullptr ? std::wstring{} :
                std::wstring(m_value, static_cast<size_t>(SysStringLen(m_value)));
        }
    private:
        BSTR m_value = nullptr;
    };

    std::string WideToUtf8(std::wstring_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int required = WideCharToMultiByte(
            CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (required <= 0)
        {
            return {};
        }

        std::string result(static_cast<size_t>(required), '\0');
        WideCharToMultiByte(
            CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), required, nullptr, nullptr);
        return result;
    }

    std::wstring Utf8ToWide(std::string_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int required = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (required <= 0)
        {
            return {};
        }

        std::wstring result(static_cast<size_t>(required), L'\0');
        MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), required);
        return result;
    }

    std::wstring Trim(std::wstring value)
    {
        const auto isSpace = [](wchar_t character) {
            return std::iswspace(static_cast<wint_t>(character)) != 0;
        };
        value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), isSpace));
        value.erase(std::find_if_not(value.rbegin(), value.rend(), isSpace).base(), value.end());
        return value;
    }

    std::wstring ToLower(std::wstring value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](wchar_t character) {
            return static_cast<wchar_t>(std::towlower(static_cast<wint_t>(character)));
        });
        return value;
    }

    std::string StableId(std::string_view prefix, std::wstring identity)
    {
        identity = ToLower(std::move(identity));
        unsigned long long hash = 14695981039346656037ULL;
        for (const wchar_t character : identity)
        {
            const unsigned int value = static_cast<unsigned int>(character);
            hash ^= static_cast<unsigned char>(value & 0xffU);
            hash *= 1099511628211ULL;
            hash ^= static_cast<unsigned char>((value >> 8U) & 0xffU);
            hash *= 1099511628211ULL;
        }

        std::ostringstream stream;
        stream << prefix << '-' << std::hex << std::setw(16) << std::setfill('0') << hash;
        return stream.str();
    }

    std::wstring RegistrySourceIdentity(
        bool currentUser,
        unsigned long view,
        std::wstring_view keyPath,
        std::wstring_view valueName)
    {
        return std::wstring(currentUser ? L"HKCU|" : L"HKLM|") + std::to_wstring(view) +
            L"|" + std::wstring(keyPath) + L"|" + std::wstring(valueName);
    }

    std::wstring StartupSourceIdentity(bool currentUser, const std::filesystem::path& originalPath)
    {
        return std::wstring(currentUser ? L"current|" : L"common|") +
            originalPath.lexically_normal().wstring();
    }

    std::wstring TaskSourceIdentity(std::wstring_view taskPath)
    {
        return L"task|" + std::wstring(taskPath);
    }

    std::wstring ServiceSourceIdentity(std::wstring_view serviceName)
    {
        return L"service|" + std::wstring(serviceName);
    }

    std::wstring DriverSourceIdentity(std::wstring_view serviceName)
    {
        return L"driver|" + std::wstring(serviceName);
    }

    std::wstring SourceIdentity(const wpcc::AutorunEntry& entry)
    {
        if (entry.sourceType == wpcc::AutorunSourceType::RegistryValue)
        {
            return RegistrySourceIdentity(
                entry.registryCurrentUser,
                entry.registryView,
                entry.registryKeyPath,
                entry.registryValueName);
        }
        if (entry.sourceType == wpcc::AutorunSourceType::StartupFolder)
        {
            return StartupSourceIdentity(entry.startupCurrentUser, entry.startupOriginalPath);
        }
        if (entry.sourceType == wpcc::AutorunSourceType::ScheduledTask)
        {
            return TaskSourceIdentity(entry.taskPath);
        }
        if (entry.sourceType == wpcc::AutorunSourceType::Service)
        {
            return ServiceSourceIdentity(entry.serviceName);
        }
        return DriverSourceIdentity(entry.serviceName);
    }

    bool SameSourceIdentity(const wpcc::AutorunEntry& left, const wpcc::AutorunEntry& right)
    {
        return left.sourceType == right.sourceType &&
            ToLower(SourceIdentity(left)) == ToLower(SourceIdentity(right));
    }

    void AppendWarning(std::wstring& target, std::wstring_view warning)
    {
        if (warning.empty())
        {
            return;
        }
        if (!target.empty())
        {
            target += L" ";
        }
        target += warning;
    }

    bool Is64BitWindows()
    {
        SYSTEM_INFO systemInfo{};
        GetNativeSystemInfo(&systemInfo);
        return systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
            systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64 ||
            systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64;
    }

    std::vector<std::pair<REGSAM, std::wstring>> RegistryViews()
    {
        if (Is64BitWindows())
        {
            return {
                {KEY_WOW64_64KEY, L"64-bit"},
                {KEY_WOW64_32KEY, L"32-bit"},
            };
        }
        return {{0, L"native"}};
    }

    std::wstring RegistryString(const std::vector<unsigned char>& bytes)
    {
        if (bytes.size() < sizeof(wchar_t))
        {
            return {};
        }

        const size_t characterCount = bytes.size() / sizeof(wchar_t);
        std::wstring value(characterCount, L'\0');
        std::memcpy(value.data(), bytes.data(), characterCount * sizeof(wchar_t));
        const size_t terminator = value.find(L'\0');
        if (terminator != std::wstring::npos)
        {
            value.resize(terminator);
        }
        return value;
    }

    std::wstring ExpandEnvironment(std::wstring_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const DWORD required = ExpandEnvironmentStringsW(std::wstring(value).c_str(), nullptr, 0);
        if (required == 0)
        {
            return std::wstring(value);
        }

        std::wstring expanded(static_cast<size_t>(required), L'\0');
        const DWORD written = ExpandEnvironmentStringsW(
            std::wstring(value).c_str(), expanded.data(), required);
        if (written == 0 || written > required)
        {
            return std::wstring(value);
        }
        expanded.resize(written - 1);
        return expanded;
    }

    bool FileExists(std::wstring_view path)
    {
        if (path.empty())
        {
            return false;
        }
        const DWORD attributes = GetFileAttributesW(std::wstring(path).c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    struct ParsedImage
    {
        std::wstring path;
        bool resolved = false;
    };

    ParsedImage ParseCommandImage(std::wstring_view originalCommand)
    {
        const std::wstring command = Trim(ExpandEnvironment(originalCommand));
        if (command.empty())
        {
            return {};
        }

        ParsedImage result{};
        if (command.front() == L'"')
        {
            const size_t closingQuote = command.find(L'"', 1);
            if (closingQuote == std::wstring::npos)
            {
                return {};
            }
            result.path = Trim(command.substr(1, closingQuote - 1));
            result.resolved = !result.path.empty();
        }
        else
        {
            const std::wstring lower = ToLower(command);
            size_t executableEnd = std::wstring::npos;
            for (const std::wstring_view extension : {L".exe", L".com", L".bat", L".cmd", L".sys"})
            {
                size_t searchAt = 0;
                while (searchAt < lower.size())
                {
                    const size_t found = lower.find(extension, searchAt);
                    if (found == std::wstring::npos)
                    {
                        break;
                    }
                    const size_t end = found + extension.size();
                    if (end == lower.size() || std::iswspace(static_cast<wint_t>(lower[end])) != 0)
                    {
                        executableEnd = std::min(executableEnd, end);
                        break;
                    }
                    searchAt = found + 1;
                }
            }

            if (executableEnd != std::wstring::npos)
            {
                result.path = Trim(command.substr(0, executableEnd));
                result.resolved = !result.path.empty();
            }
            else
            {
                const size_t separator = command.find_first_of(L" \t\r\n");
                result.path = command.substr(0, separator);
                result.resolved = false;
            }
        }

        if (result.resolved && !result.path.empty())
        {
            std::wstring normalized = result.path;
            const std::wstring lower = ToLower(normalized);
            std::array<wchar_t, MAX_PATH> windowsDirectory{};
            const UINT windowsLength = GetWindowsDirectoryW(
                windowsDirectory.data(), static_cast<UINT>(windowsDirectory.size()));
            if (windowsLength > 0 && windowsLength < windowsDirectory.size())
            {
                const std::wstring windowsPath(windowsDirectory.data(), windowsLength);
                if (lower.starts_with(L"\\systemroot\\"))
                {
                    normalized = windowsPath + normalized.substr(11);
                }
                else if (lower.starts_with(L"system32\\"))
                {
                    normalized = windowsPath + L"\\" + normalized;
                }
                else if (lower.starts_with(L"\\??\\"))
                {
                    normalized.erase(0, 4);
                }
                result.path = std::move(normalized);
            }
        }

        if (result.resolved && !result.path.empty() &&
            result.path.find_first_of(L"\\/") == std::wstring::npos)
        {
            const DWORD required = SearchPathW(nullptr, result.path.c_str(), nullptr, 0, nullptr, nullptr);
            if (required > 0)
            {
                std::wstring resolvedPath(static_cast<size_t>(required) + 1, L'\0');
                const DWORD written = SearchPathW(
                    nullptr, result.path.c_str(), nullptr,
                    static_cast<DWORD>(resolvedPath.size()), resolvedPath.data(), nullptr);
                if (written > 0 && written < resolvedPath.size())
                {
                    resolvedPath.resize(written);
                    result.path = std::move(resolvedPath);
                }
            }
        }

        return result;
    }

    std::wstring StatusForImage(const ParsedImage& image)
    {
        if (image.path.empty() || !image.resolved)
        {
            return L"Unresolved command";
        }
        return FileExists(image.path) ? L"OK" : L"File not found";
    }

    std::wstring CurrentUserLabel()
    {
        DWORD length = 0;
        GetUserNameW(nullptr, &length);
        if (length == 0)
        {
            return L"Current user";
        }

        std::wstring name(static_cast<size_t>(length), L'\0');
        if (!GetUserNameW(name.data(), &length) || length == 0)
        {
            return L"Current user";
        }
        name.resize(length - 1);
        return name;
    }

    std::filesystem::path AppDataDirectory()
    {
        PWSTR rawPath = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &rawPath)) ||
            rawPath == nullptr)
        {
            return {};
        }
        const std::filesystem::path result = std::filesystem::path(rawPath) / L"WindowsProcessControlCenter";
        CoTaskMemFree(rawPath);
        return result;
    }

    std::filesystem::path KnownFolderPath(REFKNOWNFOLDERID folderId)
    {
        PWSTR rawPath = nullptr;
        if (FAILED(SHGetKnownFolderPath(folderId, KF_FLAG_DEFAULT, nullptr, &rawPath)) || rawPath == nullptr)
        {
            return {};
        }
        const std::filesystem::path result(rawPath);
        CoTaskMemFree(rawPath);
        return result;
    }

    bool PathsEqual(const std::filesystem::path& left, const std::filesystem::path& right)
    {
        return ToLower(left.lexically_normal().wstring()) == ToLower(right.lexically_normal().wstring());
    }

    std::filesystem::path DisabledStorePath()
    {
        const std::filesystem::path directory = AppDataDirectory();
        return directory.empty() ? std::filesystem::path{} : directory / L"disabled-autoruns.json";
    }

    std::filesystem::path DisabledFilesDirectory()
    {
        const std::filesystem::path directory = AppDataDirectory();
        return directory.empty() ? std::filesystem::path{} : directory / L"DisabledAutoruns";
    }

    std::string HexEncode(const std::vector<unsigned char>& bytes)
    {
        static constexpr char Digits[] = "0123456789abcdef";
        std::string encoded;
        encoded.resize(bytes.size() * 2);
        for (size_t index = 0; index < bytes.size(); ++index)
        {
            encoded[index * 2] = Digits[bytes[index] >> 4U];
            encoded[index * 2 + 1] = Digits[bytes[index] & 0x0fU];
        }
        return encoded;
    }

    std::optional<std::vector<unsigned char>> HexDecode(std::string_view encoded)
    {
        if ((encoded.size() % 2) != 0)
        {
            return std::nullopt;
        }

        const auto digit = [](char character) -> int {
            if (character >= '0' && character <= '9') return character - '0';
            if (character >= 'a' && character <= 'f') return character - 'a' + 10;
            if (character >= 'A' && character <= 'F') return character - 'A' + 10;
            return -1;
        };

        std::vector<unsigned char> bytes(encoded.size() / 2);
        for (size_t index = 0; index < bytes.size(); ++index)
        {
            const int high = digit(encoded[index * 2]);
            const int low = digit(encoded[index * 2 + 1]);
            if (high < 0 || low < 0)
            {
                return std::nullopt;
            }
            bytes[index] = static_cast<unsigned char>((high << 4) | low);
        }
        return bytes;
    }

    json SerializeDisabledEntry(const wpcc::AutorunEntry& entry)
    {
        const char* sourceType = "startupFolder";
        if (entry.sourceType == wpcc::AutorunSourceType::RegistryValue)
        {
            sourceType = "registryValue";
        }
        else if (entry.sourceType == wpcc::AutorunSourceType::Service)
        {
            sourceType = "service";
        }
        json item = {
            {"id", entry.id},
            {"sourceType", sourceType},
            {"entryName", WideToUtf8(entry.entryName)},
            {"command", WideToUtf8(entry.command)},
            {"imagePath", WideToUtf8(entry.imagePath)},
            {"location", WideToUtf8(entry.location)},
            {"user", WideToUtf8(entry.user)},
            {"requiresElevation", entry.requiresElevation},
        };

        if (entry.sourceType == wpcc::AutorunSourceType::RegistryValue)
        {
            item["registry"] = {
                {"root", entry.registryCurrentUser ? "HKCU" : "HKLM"},
                {"view", entry.registryView},
                {"keyPath", WideToUtf8(entry.registryKeyPath)},
                {"valueName", WideToUtf8(entry.registryValueName)},
                {"valueType", entry.registryValueType},
                {"valueDataHex", HexEncode(entry.registryValueData)},
            };
        }
        else if (entry.sourceType == wpcc::AutorunSourceType::StartupFolder)
        {
            item["startup"] = {
                {"scope", entry.startupCurrentUser ? "currentUser" : "common"},
                {"originalPath", WideToUtf8(entry.startupOriginalPath)},
                {"disabledPath", WideToUtf8(entry.startupDisabledPath)},
            };
        }
        else if (entry.sourceType == wpcc::AutorunSourceType::Service)
        {
            item["service"] = {
                {"name", WideToUtf8(entry.serviceName)},
                {"originalStartType", entry.serviceOriginalStartType},
            };
        }
        return item;
    }

    bool DeserializeDisabledEntry(const json& item, wpcc::AutorunEntry& entry)
    {
        if (!item.is_object() || !item.contains("id") || !item["id"].is_string() ||
            !item.contains("sourceType") || !item["sourceType"].is_string())
        {
            return false;
        }

        entry.id = item.at("id").get<std::string>();
        entry.category = wpcc::AutorunCategory::Logon;
        entry.enabled = false;
        entry.status = L"Disabled by WPCC";
        entry.entryName = Utf8ToWide(item.value("entryName", std::string{}));
        entry.command = Utf8ToWide(item.value("command", std::string{}));
        entry.imagePath = Utf8ToWide(item.value("imagePath", std::string{}));
        entry.location = Utf8ToWide(item.value("location", std::string{}));
        entry.user = Utf8ToWide(item.value("user", std::string{}));
        entry.requiresElevation = item.value("requiresElevation", false);

        const std::string sourceType = item.at("sourceType").get<std::string>();
        if (sourceType == "registryValue" && item.contains("registry") && item["registry"].is_object())
        {
            const json& registry = item["registry"];
            const std::string root = registry.at("root").get<std::string>();
            const std::string dataHex = registry.at("valueDataHex").get<std::string>();
            const auto decoded = HexDecode(dataHex);
            if ((root != "HKCU" && root != "HKLM") || !decoded)
            {
                return false;
            }
            entry.sourceType = wpcc::AutorunSourceType::RegistryValue;
            entry.registryCurrentUser = root == "HKCU";
            entry.registryView = registry.at("view").get<unsigned long>();
            entry.registryKeyPath = Utf8ToWide(registry.at("keyPath").get<std::string>());
            entry.registryValueName = Utf8ToWide(registry.at("valueName").get<std::string>());
            entry.registryValueType = registry.at("valueType").get<unsigned long>();
            entry.registryValueData = *decoded;
            const std::wstring normalizedKeyPath = ToLower(entry.registryKeyPath);
            const bool validKeyPath = normalizedKeyPath == ToLower(RunKey) ||
                normalizedKeyPath == ToLower(RunOnceKey);
            const bool validView = entry.registryView == 0 ||
                entry.registryView == KEY_WOW64_64KEY || entry.registryView == KEY_WOW64_32KEY;
            const std::wstring identity = RegistrySourceIdentity(
                entry.registryCurrentUser,
                entry.registryView,
                entry.registryKeyPath,
                entry.registryValueName);
            entry.requiresElevation = !entry.registryCurrentUser;
            entry.entryName = entry.registryValueName.empty() ? L"(Default)" : entry.registryValueName;
            return entry.id == StableId("reg", identity) && validKeyPath && validView;
        }

        if (sourceType == "startupFolder" && item.contains("startup") && item["startup"].is_object())
        {
            const json& startup = item["startup"];
            const std::string scope = startup.at("scope").get<std::string>();
            if (scope != "currentUser" && scope != "common")
            {
                return false;
            }
            entry.sourceType = wpcc::AutorunSourceType::StartupFolder;
            entry.startupCurrentUser = scope == "currentUser";
            entry.startupOriginalPath = Utf8ToWide(startup.at("originalPath").get<std::string>());
            entry.startupDisabledPath = Utf8ToWide(startup.at("disabledPath").get<std::string>());
            const std::filesystem::path expectedStartupDirectory = KnownFolderPath(
                entry.startupCurrentUser ? FOLDERID_Startup : FOLDERID_CommonStartup);
            const std::filesystem::path expectedDisabledDirectory = DisabledFilesDirectory();
            const std::filesystem::path originalPath(entry.startupOriginalPath);
            const std::filesystem::path disabledPath(entry.startupDisabledPath);
            const std::filesystem::path expectedDisabledPath = expectedDisabledDirectory /
                (Utf8ToWide(entry.id) + originalPath.extension().wstring());
            const std::wstring identity = StartupSourceIdentity(entry.startupCurrentUser, originalPath);
            entry.requiresElevation = !entry.startupCurrentUser;
            entry.entryName = originalPath.stem().wstring();
            return entry.id == StableId("startup", identity) &&
                !entry.startupOriginalPath.empty() && !entry.startupDisabledPath.empty() &&
                !expectedStartupDirectory.empty() && !expectedDisabledDirectory.empty() &&
                PathsEqual(originalPath.parent_path(), expectedStartupDirectory) &&
                PathsEqual(disabledPath, expectedDisabledPath);
        }
        if (sourceType == "service" && item.contains("service") && item["service"].is_object())
        {
            const json& service = item["service"];
            entry.category = wpcc::AutorunCategory::Service;
            entry.sourceType = wpcc::AutorunSourceType::Service;
            entry.serviceName = Utf8ToWide(service.at("name").get<std::string>());
            entry.serviceOriginalStartType = service.at("originalStartType").get<unsigned long>();
            const bool validStartType = entry.serviceOriginalStartType == SERVICE_BOOT_START ||
                entry.serviceOriginalStartType == SERVICE_SYSTEM_START ||
                entry.serviceOriginalStartType == SERVICE_AUTO_START ||
                entry.serviceOriginalStartType == SERVICE_DEMAND_START;
            entry.requiresElevation = true;
            entry.entryName = entry.entryName.empty() ? entry.serviceName : entry.entryName;
            return !entry.serviceName.empty() && validStartType &&
                entry.id == StableId("service", ServiceSourceIdentity(entry.serviceName));
        }
        return false;
    }

    bool LoadDisabledEntries(std::vector<wpcc::AutorunEntry>& entries, std::wstring& warning)
    {
        entries.clear();
        const std::filesystem::path path = DisabledStorePath();
        if (path.empty())
        {
            warning = L"Unable to locate the Autoruns data directory.";
            return false;
        }

        std::error_code error;
        if (!std::filesystem::exists(path, error))
        {
            if (error)
            {
                warning = L"The disabled Autoruns store could not be checked.";
                return false;
            }
            return true;
        }

        try
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                warning = L"The disabled Autoruns store could not be read.";
                return false;
            }
            json document;
            file >> document;
            if (!document.is_object() || document.value("schemaVersion", 0) != DisabledStoreSchemaVersion ||
                !document.contains("entries") || !document["entries"].is_array())
            {
                warning = L"The disabled Autoruns store has an unsupported format.";
                return false;
            }

            for (const json& item : document["entries"])
            {
                wpcc::AutorunEntry entry;
                if (!DeserializeDisabledEntry(item, entry))
                {
                    warning = L"The disabled Autoruns store contains invalid entry metadata.";
                    entries.clear();
                    return false;
                }
                entries.push_back(std::move(entry));
            }
            return true;
        }
        catch (...)
        {
            warning = L"The disabled Autoruns store contains invalid JSON.";
            entries.clear();
            return false;
        }
    }

    bool SaveDisabledEntries(const std::vector<wpcc::AutorunEntry>& entries, std::wstring& warning)
    {
        const std::filesystem::path path = DisabledStorePath();
        if (path.empty())
        {
            warning = L"Unable to locate the Autoruns data directory.";
            return false;
        }

        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error)
        {
            warning = L"The Autoruns data directory could not be created.";
            return false;
        }

        json document = {{"schemaVersion", DisabledStoreSchemaVersion}, {"entries", json::array()}};
        for (const wpcc::AutorunEntry& entry : entries)
        {
            document["entries"].push_back(SerializeDisabledEntry(entry));
        }

        const std::filesystem::path temporaryPath = path.wstring() + L".tmp";
        try
        {
            std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);
            if (!file)
            {
                warning = L"The disabled Autoruns store could not be written.";
                return false;
            }
            file << document.dump(2);
            file.flush();
            if (!file)
            {
                file.close();
                DeleteFileW(temporaryPath.c_str());
                warning = L"The disabled Autoruns store could not be written completely.";
                return false;
            }
            file.close();
        }
        catch (...)
        {
            DeleteFileW(temporaryPath.c_str());
            warning = L"The disabled Autoruns store could not be serialized.";
            return false;
        }

        if (!MoveFileExW(
            temporaryPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            DeleteFileW(temporaryPath.c_str());
            warning = L"The disabled Autoruns store could not be committed.";
            return false;
        }
        return true;
    }

    LSTATUS QueryRegistryValue(
        HKEY key,
        std::wstring_view valueName,
        unsigned long& type,
        std::vector<unsigned char>& data)
    {
        DWORD byteCount = 0;
        DWORD nativeType = 0;
        const std::wstring name(valueName);
        LSTATUS status = RegQueryValueExW(key, name.c_str(), nullptr, &nativeType, nullptr, &byteCount);
        if (status != ERROR_SUCCESS)
        {
            return status;
        }

        data.assign(byteCount, 0);
        status = RegQueryValueExW(
            key, name.c_str(), nullptr, &nativeType,
            data.empty() ? nullptr : data.data(), &byteCount);
        if (status == ERROR_SUCCESS)
        {
            data.resize(byteCount);
            type = nativeType;
        }
        return status;
    }

    std::wstring RegistryLocationLabel(
        bool currentUser,
        std::wstring_view keyPath,
        std::wstring_view viewLabel)
    {
        return std::wstring(currentUser ? L"HKCU\\" : L"HKLM\\") + std::wstring(keyPath) +
            L" (" + std::wstring(viewLabel) + L")";
    }

    void EnumerateRegistryLocation(
        bool currentUser,
        std::wstring_view keyPath,
        REGSAM view,
        std::wstring_view viewLabel,
        std::vector<wpcc::AutorunEntry>& entries,
        std::wstring& warning)
    {
        const HKEY root = currentUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
        RegistryKey key;
        const LSTATUS openStatus = RegOpenKeyExW(
            root, std::wstring(keyPath).c_str(), 0, KEY_QUERY_VALUE | view, key.Put());
        if (openStatus == ERROR_FILE_NOT_FOUND)
        {
            return;
        }
        if (openStatus != ERROR_SUCCESS)
        {
            AppendWarning(warning, std::wstring(currentUser ? L"A current-user" : L"An all-users") +
                L" Logon registry location could not be read.");
            return;
        }

        DWORD valueCount = 0;
        DWORD maximumNameLength = 0;
        DWORD maximumDataLength = 0;
        const LSTATUS infoStatus = RegQueryInfoKeyW(
            key.Get(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            &valueCount, &maximumNameLength, &maximumDataLength, nullptr, nullptr);
        if (infoStatus != ERROR_SUCCESS)
        {
            AppendWarning(warning, L"A Logon registry location could not be enumerated.");
            return;
        }

        for (DWORD index = 0; index < valueCount; ++index)
        {
            std::wstring valueName(static_cast<size_t>(maximumNameLength) + 1, L'\0');
            std::vector<unsigned char> valueData(std::max<DWORD>(maximumDataLength, 1));
            DWORD nameLength = static_cast<DWORD>(valueName.size());
            DWORD dataLength = static_cast<DWORD>(valueData.size());
            DWORD valueType = 0;
            const LSTATUS enumStatus = RegEnumValueW(
                key.Get(), index, valueName.data(), &nameLength, nullptr, &valueType,
                valueData.data(), &dataLength);
            if (enumStatus != ERROR_SUCCESS)
            {
                continue;
            }
            valueName.resize(nameLength);
            valueData.resize(dataLength);

            wpcc::AutorunEntry entry;
            entry.category = wpcc::AutorunCategory::Logon;
            entry.sourceType = wpcc::AutorunSourceType::RegistryValue;
            entry.entryName = valueName.empty() ? L"(Default)" : valueName;
            entry.registryCurrentUser = currentUser;
            entry.registryView = view;
            entry.registryKeyPath = keyPath;
            entry.registryValueName = valueName;
            entry.registryValueType = valueType;
            entry.registryValueData = valueData;
            entry.location = RegistryLocationLabel(currentUser, keyPath, viewLabel);
            entry.user = currentUser ? CurrentUserLabel() : L"All users";
            entry.enabled = true;
            entry.requiresElevation = !currentUser;

            if (valueType == REG_SZ || valueType == REG_EXPAND_SZ)
            {
                entry.command = RegistryString(valueData);
            }
            const ParsedImage parsed = ParseCommandImage(entry.command);
            entry.imagePath = parsed.path;
            entry.status = StatusForImage(parsed);

            const std::wstring identity = RegistrySourceIdentity(currentUser, view, keyPath, valueName);
            entry.id = StableId("reg", identity);
            entries.push_back(std::move(entry));
        }
    }

    bool ResolveShortcut(
        std::wstring_view shortcutPath,
        std::wstring& command,
        std::wstring& imagePath)
    {
        ComPtr<IShellLinkW> shellLink;
        if (FAILED(CoCreateInstance(
            CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink))) || !shellLink)
        {
            return false;
        }

        ComPtr<IPersistFile> persistFile;
        if (FAILED(shellLink.As(&persistFile)) || !persistFile ||
            FAILED(persistFile->Load(std::wstring(shortcutPath).c_str(), STGM_READ)))
        {
            return false;
        }

        std::array<wchar_t, 32768> target{};
        WIN32_FIND_DATAW findData{};
        if (FAILED(shellLink->GetPath(
            target.data(), static_cast<int>(target.size()), &findData, SLGP_RAWPATH)) || target[0] == L'\0')
        {
            return false;
        }

        std::array<wchar_t, 32768> arguments{};
        shellLink->GetArguments(arguments.data(), static_cast<int>(arguments.size()));
        imagePath = ExpandEnvironment(target.data());
        command = L"\"" + imagePath + L"\"";
        if (arguments[0] != L'\0')
        {
            command += L" ";
            command += arguments.data();
        }
        return true;
    }

    void EnumerateStartupFolder(
        REFKNOWNFOLDERID folderId,
        bool currentUser,
        std::vector<wpcc::AutorunEntry>& entries,
        std::wstring& warning)
    {
        PWSTR rawFolderPath = nullptr;
        if (FAILED(SHGetKnownFolderPath(folderId, KF_FLAG_DEFAULT, nullptr, &rawFolderPath)) || rawFolderPath == nullptr)
        {
            AppendWarning(warning, std::wstring(currentUser ? L"The current-user" : L"The common") +
                L" Startup folder could not be located.");
            return;
        }
        const std::filesystem::path folderPath(rawFolderPath);
        CoTaskMemFree(rawFolderPath);

        const std::filesystem::path searchPath = folderPath / L"*";
        WIN32_FIND_DATAW findData{};
        HANDLE findHandle = FindFirstFileW(searchPath.c_str(), &findData);
        if (findHandle == INVALID_HANDLE_VALUE)
        {
            const DWORD error = GetLastError();
            if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
            {
                AppendWarning(warning, std::wstring(currentUser ? L"The current-user" : L"The common") +
                    L" Startup folder could not be read.");
            }
            return;
        }
        const FindFileHandle scopedFindHandle(findHandle);

        do
        {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ||
                std::wstring_view(findData.cFileName) == L"." || std::wstring_view(findData.cFileName) == L"..")
            {
                continue;
            }

            const std::filesystem::path itemPath = folderPath / findData.cFileName;
            wpcc::AutorunEntry entry;
            entry.category = wpcc::AutorunCategory::Logon;
            entry.sourceType = wpcc::AutorunSourceType::StartupFolder;
            entry.entryName = itemPath.stem().wstring();
            entry.location = folderPath.wstring();
            entry.user = currentUser ? CurrentUserLabel() : L"All users";
            entry.enabled = true;
            entry.requiresElevation = !currentUser;
            entry.startupOriginalPath = itemPath.lexically_normal().wstring();
            entry.startupCurrentUser = currentUser;

            if (ToLower(itemPath.extension().wstring()) == L".lnk")
            {
                if (ResolveShortcut(itemPath.wstring(), entry.command, entry.imagePath))
                {
                    entry.status = FileExists(entry.imagePath) ? L"OK" : L"File not found";
                }
                else
                {
                    entry.command = itemPath.wstring();
                    entry.imagePath = itemPath.wstring();
                    entry.status = L"Unresolved command";
                }
            }
            else
            {
                entry.command = itemPath.wstring();
                entry.imagePath = itemPath.wstring();
                entry.status = L"OK";
            }

            const std::wstring identity = StartupSourceIdentity(currentUser, itemPath);
            entry.id = StableId("startup", identity);
            entries.push_back(std::move(entry));
        } while (FindNextFileW(scopedFindHandle.Get(), &findData));
    }

    unsigned long ErrorFromHresult(HRESULT result)
    {
        return HRESULT_FACILITY(result) == FACILITY_WIN32 ? HRESULT_CODE(result) :
            static_cast<unsigned long>(result);
    }

    HRESULT ConnectTaskScheduler(ComPtr<ITaskService>& service)
    {
        HRESULT result = CoCreateInstance(
            CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&service));
        if (FAILED(result))
        {
            return result;
        }

        VARIANT empty;
        VariantInit(&empty);
        return service->Connect(empty, empty, empty, empty);
    }

    std::wstring CombineTaskCommand(std::wstring_view path, std::wstring_view arguments)
    {
        if (path.empty())
        {
            return {};
        }
        std::wstring command = L"\"" + std::wstring(path) + L"\"";
        if (!arguments.empty())
        {
            command += L" ";
            command += arguments;
        }
        return command;
    }

    bool AddScheduledTask(IRegisteredTask* task, std::vector<wpcc::AutorunEntry>& entries)
    {
        if (task == nullptr)
        {
            return false;
        }

        wpcc::AutorunEntry entry;
        entry.category = wpcc::AutorunCategory::ScheduledTask;
        entry.sourceType = wpcc::AutorunSourceType::ScheduledTask;
        entry.requiresElevation = false;

        ScopedBstr name;
        ScopedBstr path;
        if (SUCCEEDED(task->get_Name(name.Put()))) entry.entryName = name.String();
        if (FAILED(task->get_Path(path.Put())) || path.String().empty()) return false;
        entry.taskPath = path.String();
        entry.location = entry.taskPath;

        VARIANT_BOOL nativeEnabled = VARIANT_FALSE;
        if (SUCCEEDED(task->get_Enabled(&nativeEnabled)))
        {
            entry.enabled = nativeEnabled == VARIANT_TRUE;
        }
        else
        {
            entry.enabledKnown = false;
            entry.canSetEnabled = false;
            entry.readOnlyReason = L"Task enabled state is unavailable.";
        }

        long execActionCount = 0;
        long nonExecActionCount = 0;
        std::vector<std::wstring> commands;
        ComPtr<ITaskDefinition> definition;
        if (SUCCEEDED(task->get_Definition(&definition)) && definition)
        {
            ComPtr<IPrincipal> principal;
            if (SUCCEEDED(definition->get_Principal(&principal)) && principal)
            {
                ScopedBstr userId;
                ScopedBstr groupId;
                if (SUCCEEDED(principal->get_UserId(userId.Put())) && !userId.String().empty())
                {
                    entry.user = userId.String();
                }
                else if (SUCCEEDED(principal->get_GroupId(groupId.Put())))
                {
                    entry.user = groupId.String();
                }
            }

            ComPtr<IActionCollection> actions;
            if (SUCCEEDED(definition->get_Actions(&actions)) && actions)
            {
                long count = 0;
                if (SUCCEEDED(actions->get_Count(&count)))
                {
                    for (long index = 1; index <= count; ++index)
                    {
                        ComPtr<IAction> action;
                        if (FAILED(actions->get_Item(index, &action)) || !action)
                        {
                            continue;
                        }
                        TASK_ACTION_TYPE type = TASK_ACTION_EXEC;
                        if (FAILED(action->get_Type(&type)) || type != TASK_ACTION_EXEC)
                        {
                            ++nonExecActionCount;
                            continue;
                        }
                        ++execActionCount;
                        ComPtr<IExecAction> execAction;
                        if (FAILED(action.As(&execAction)) || !execAction)
                        {
                            continue;
                        }
                        ScopedBstr executable;
                        ScopedBstr arguments;
                        execAction->get_Path(executable.Put());
                        execAction->get_Arguments(arguments.Put());
                        const std::wstring command = CombineTaskCommand(executable.String(), arguments.String());
                        if (!command.empty()) commands.push_back(command);
                        if (entry.imagePath.empty()) entry.imagePath = ExpandEnvironment(executable.String());
                    }
                }
            }
        }

        for (size_t index = 0; index < commands.size(); ++index)
        {
            if (index > 0) entry.command += L" | ";
            entry.command += commands[index];
        }
        if (entry.entryName.empty()) entry.entryName = entry.taskPath;
        if (!entry.enabledKnown) entry.status = L"Enabled state unavailable";
        else if (execActionCount > 0 && nonExecActionCount > 0)
        {
            entry.status = std::to_wstring(execActionCount) + L" Exec action" + (execActionCount == 1 ? L"" : L"s") +
                L" + " + std::to_wstring(nonExecActionCount) + L" non-exec action" +
                (nonExecActionCount == 1 ? L"" : L"s");
            if (!entry.enabled) entry.status = L"Disabled - " + entry.status;
        }
        else if (execActionCount > 1) entry.status = entry.enabled ? L"Multiple Exec actions" : L"Disabled - Multiple Exec actions";
        else if (execActionCount == 0 && nonExecActionCount > 0) entry.status = entry.enabled ? L"Non-exec task" : L"Disabled - Non-exec task";
        else if (entry.imagePath.empty()) entry.status = entry.enabled ? L"Action unavailable" : L"Disabled";
        else entry.status = entry.enabled ? StatusForImage(ParseCommandImage(entry.command)) : L"Disabled";
        entry.id = StableId("task", TaskSourceIdentity(entry.taskPath));
        entries.push_back(std::move(entry));
        return true;
    }

    void EnumerateTaskFolder(
        ITaskFolder* folder,
        std::vector<wpcc::AutorunEntry>& entries,
        bool& hadPartialFailure)
    {
        if (folder == nullptr) return;

        ComPtr<IRegisteredTaskCollection> tasks;
        if (SUCCEEDED(folder->GetTasks(TASK_ENUM_HIDDEN, &tasks)) && tasks)
        {
            long count = 0;
            if (SUCCEEDED(tasks->get_Count(&count)))
            {
                for (long index = 1; index <= count; ++index)
                {
                    VARIANT item;
                    VariantInit(&item);
                    item.vt = VT_I4;
                    item.lVal = index;
                    ComPtr<IRegisteredTask> task;
                    if (SUCCEEDED(tasks->get_Item(item, &task)) && task)
                    {
                        if (!AddScheduledTask(task.Get(), entries)) hadPartialFailure = true;
                    }
                    else hadPartialFailure = true;
                }
            }
            else hadPartialFailure = true;
        }
        else hadPartialFailure = true;

        ComPtr<ITaskFolderCollection> folders;
        if (FAILED(folder->GetFolders(0, &folders)) || !folders)
        {
            hadPartialFailure = true;
            return;
        }
        long count = 0;
        if (FAILED(folders->get_Count(&count)))
        {
            hadPartialFailure = true;
            return;
        }
        for (long index = 1; index <= count; ++index)
        {
            VARIANT item;
            VariantInit(&item);
            item.vt = VT_I4;
            item.lVal = index;
            ComPtr<ITaskFolder> child;
            if (SUCCEEDED(folders->get_Item(item, &child)) && child)
            {
                EnumerateTaskFolder(child.Get(), entries, hadPartialFailure);
            }
            else hadPartialFailure = true;
        }
    }

    void EnumerateScheduledTasks(std::vector<wpcc::AutorunEntry>& entries, std::wstring& warning)
    {
        ComPtr<ITaskService> service;
        const HRESULT connectResult = ConnectTaskScheduler(service);
        if (FAILED(connectResult) || !service)
        {
            AppendWarning(warning, L"Scheduled Tasks could not be read.");
            return;
        }
        ScopedBstr rootPath(L"\\");
        ComPtr<ITaskFolder> root;
        if (FAILED(service->GetFolder(rootPath.Get(), &root)) || !root)
        {
            AppendWarning(warning, L"The Task Scheduler root folder could not be read.");
            return;
        }
        bool hadPartialFailure = false;
        EnumerateTaskFolder(root.Get(), entries, hadPartialFailure);
        if (hadPartialFailure)
        {
            AppendWarning(warning, L"Some Scheduled Tasks or folders could not be read.");
        }
    }

    std::wstring ServiceStartTypeLabel(DWORD startType)
    {
        switch (startType)
        {
        case SERVICE_BOOT_START: return L"Boot";
        case SERVICE_SYSTEM_START: return L"System";
        case SERVICE_AUTO_START: return L"Automatic";
        case SERVICE_DEMAND_START: return L"Manual";
        case SERVICE_DISABLED: return L"Disabled";
        default: return L"Unknown start type";
        }
    }

    DWORD QueryServiceConfiguration(SC_HANDLE service, std::vector<unsigned char>& storage, QUERY_SERVICE_CONFIGW*& config)
    {
        DWORD required = 0;
        QueryServiceConfigW(service, nullptr, 0, &required);
        const DWORD initialError = GetLastError();
        if (required == 0 || initialError != ERROR_INSUFFICIENT_BUFFER) return initialError;
        storage.resize(required);
        config = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(storage.data());
        if (!QueryServiceConfigW(service, config, required, &required)) return GetLastError();
        return ERROR_SUCCESS;
    }

    void AddScmEntry(
        SC_HANDLE manager,
        const ENUM_SERVICE_STATUS_PROCESSW& item,
        std::vector<wpcc::AutorunEntry>& entries,
        bool& hadPartialFailure)
    {
        const bool driver = (item.ServiceStatusProcess.dwServiceType & SERVICE_DRIVER) != 0;
        wpcc::AutorunEntry entry;
        entry.category = driver ? wpcc::AutorunCategory::Driver : wpcc::AutorunCategory::Service;
        entry.sourceType = driver ? wpcc::AutorunSourceType::Driver : wpcc::AutorunSourceType::Service;
        entry.serviceName = item.lpServiceName == nullptr ? L"" : item.lpServiceName;
        entry.entryName = item.lpDisplayName == nullptr || item.lpDisplayName[0] == L'\0' ?
            entry.serviceName : item.lpDisplayName;
        entry.location = driver ? L"Service Control Manager - Driver" : L"Service Control Manager - Service";
        entry.requiresElevation = true;
        entry.canSetEnabled = !driver;
        if (driver)
        {
            entry.readOnlyReason = L"Driver startup management is read-only in this version.";
        }

        const SC_HANDLE nativeService = OpenServiceW(manager, entry.serviceName.c_str(), SERVICE_QUERY_CONFIG);
        const DWORD openError = nativeService == nullptr ? GetLastError() : ERROR_SUCCESS;
        ServiceHandle service(nativeService);
        std::vector<unsigned char> storage;
        QUERY_SERVICE_CONFIGW* config = nullptr;
        const DWORD queryError = service.Get() == nullptr ? openError :
            QueryServiceConfiguration(service.Get(), storage, config);
        if (queryError == ERROR_SUCCESS && config != nullptr)
        {
            entry.command = config->lpBinaryPathName == nullptr ? L"" : config->lpBinaryPathName;
            entry.user = config->lpServiceStartName == nullptr ? L"" : config->lpServiceStartName;
            entry.serviceStartType = config->dwStartType;
            entry.enabled = config->dwStartType != SERVICE_DISABLED;
            const ParsedImage parsed = ParseCommandImage(entry.command);
            entry.imagePath = parsed.path;
            entry.status = ServiceStartTypeLabel(config->dwStartType);
            if (driver)
            {
                entry.status += L" - Read-only";
            }
            else if (config->dwStartType == SERVICE_DISABLED)
            {
                entry.canSetEnabled = false;
                entry.readOnlyReason = L"WPCC does not know the original start type of this externally disabled service.";
            }
        }
        else
        {
            entry.enabledKnown = false;
            entry.canSetEnabled = false;
            entry.status = L"Configuration unavailable";
            entry.readOnlyReason = driver ? L"Driver startup management is read-only in this version." :
                L"Service configuration is unavailable.";
            hadPartialFailure = true;
        }

        const std::wstring identity = driver ? DriverSourceIdentity(entry.serviceName) :
            ServiceSourceIdentity(entry.serviceName);
        entry.id = StableId(driver ? "driver" : "service", identity);
        entries.push_back(std::move(entry));
    }

    void EnumerateScmEntries(std::vector<wpcc::AutorunEntry>& entries, std::wstring& warning)
    {
        ServiceHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE));
        if (manager.Get() == nullptr)
        {
            AppendWarning(warning, L"Services and drivers could not be enumerated.");
            return;
        }

        std::vector<unsigned char> buffer(256U * 1024U);
        DWORD resumeHandle = 0;
        bool hadPartialFailure = false;
        for (;;)
        {
            DWORD bytesNeeded = 0;
            DWORD entriesRead = 0;
            const BOOL complete = EnumServicesStatusExW(
                manager.Get(), SC_ENUM_PROCESS_INFO, SERVICE_WIN32 | SERVICE_DRIVER,
                SERVICE_STATE_ALL, buffer.data(), static_cast<DWORD>(buffer.size()),
                &bytesNeeded, &entriesRead, &resumeHandle, nullptr);
            const DWORD error = complete ? ERROR_SUCCESS : GetLastError();
            const auto* nativeEntries = reinterpret_cast<const ENUM_SERVICE_STATUS_PROCESSW*>(buffer.data());
            for (DWORD index = 0; index < entriesRead; ++index)
            {
                AddScmEntry(manager.Get(), nativeEntries[index], entries, hadPartialFailure);
            }
            if (complete) break;
            if (error != ERROR_MORE_DATA)
            {
                hadPartialFailure = true;
                break;
            }
            if (entriesRead == 0)
            {
                if (bytesNeeded > buffer.size()) buffer.resize(bytesNeeded);
                else
                {
                    hadPartialFailure = true;
                    break;
                }
            }
        }
        if (hadPartialFailure)
        {
            AppendWarning(warning, L"Some service or driver details could not be read.");
        }
    }

    HRESULT SetScheduledTaskEnabled(std::wstring_view taskPath, bool enabled)
    {
        ComPtr<ITaskService> service;
        HRESULT result = ConnectTaskScheduler(service);
        if (FAILED(result) || !service) return result;

        const size_t separator = taskPath.find_last_of(L'\\');
        if (separator == std::wstring_view::npos || separator + 1 >= taskPath.size()) return E_INVALIDARG;
        const std::wstring folderPath = separator == 0 ? L"\\" : std::wstring(taskPath.substr(0, separator));
        const std::wstring taskName(taskPath.substr(separator + 1));
        ScopedBstr nativeFolderPath(folderPath);
        ComPtr<ITaskFolder> folder;
        result = service->GetFolder(nativeFolderPath.Get(), &folder);
        if (FAILED(result) || !folder) return result;
        ScopedBstr nativeTaskName(taskName);
        ComPtr<IRegisteredTask> task;
        result = folder->GetTask(nativeTaskName.Get(), &task);
        if (FAILED(result) || !task) return result;
        return task->put_Enabled(enabled ? VARIANT_TRUE : VARIANT_FALSE);
    }

    wpcc::AutorunActionResult FailureResult(
        std::string_view id,
        bool enabled,
        std::string message,
        unsigned long error = 0)
    {
        if (error == ERROR_ACCESS_DENIED)
        {
            message += " Administrator privileges may be required.";
        }
        return {false, std::string(id), enabled, std::move(message), error};
    }

    wpcc::AutorunActionResult SuccessResult(
        std::string_view id,
        bool enabled,
        std::string message)
    {
        return {true, std::string(id), enabled, std::move(message), 0};
    }

    auto FindDisabledEntry(std::vector<wpcc::AutorunEntry>& entries, std::string_view id)
    {
        return std::find_if(entries.begin(), entries.end(), [id](const wpcc::AutorunEntry& entry) {
            return entry.id == id;
        });
    }
}

namespace wpcc
{
    AutorunScanResult AutorunProvider::GetLogonEntries()
    {
        AutorunScanResult result;
        m_entries.clear();
        m_ambiguousIds.clear();

        const std::vector<std::pair<REGSAM, std::wstring>> views = RegistryViews();
        const auto& [currentUserView, currentUserViewLabel] = views.front();
        const std::wstring effectiveCurrentUserViewLabel = Is64BitWindows() ? L"shared" : currentUserViewLabel;
        EnumerateRegistryLocation(
            true, RunKey, currentUserView, effectiveCurrentUserViewLabel, result.entries, result.warning);
        EnumerateRegistryLocation(
            true, RunOnceKey, currentUserView, effectiveCurrentUserViewLabel, result.entries, result.warning);

        for (const auto& [view, viewLabel] : views)
        {
            EnumerateRegistryLocation(false, RunKey, view, viewLabel, result.entries, result.warning);
            EnumerateRegistryLocation(false, RunOnceKey, view, viewLabel, result.entries, result.warning);
        }

        EnumerateStartupFolder(FOLDERID_Startup, true, result.entries, result.warning);
        EnumerateStartupFolder(FOLDERID_CommonStartup, false, result.entries, result.warning);
        try
        {
            EnumerateScheduledTasks(result.entries, result.warning);
        }
        catch (...)
        {
            AppendWarning(result.warning, L"Scheduled Tasks could not be read.");
        }
        try
        {
            EnumerateScmEntries(result.entries, result.warning);
        }
        catch (...)
        {
            AppendWarning(result.warning, L"Services and drivers could not be enumerated.");
        }

        std::vector<AutorunEntry> uniqueEntries;
        uniqueEntries.reserve(result.entries.size());
        for (const AutorunEntry& entry : result.entries)
        {
            const auto [existing, inserted] = m_entries.emplace(entry.id, entry);
            if (inserted)
            {
                uniqueEntries.push_back(entry);
            }
            else if (!SameSourceIdentity(existing->second, entry))
            {
                m_ambiguousIds.insert(entry.id);
                uniqueEntries.push_back(entry);
            }
        }
        result.entries = std::move(uniqueEntries);

        std::vector<AutorunEntry> disabledEntries;
        std::wstring storeWarning;
        if (LoadDisabledEntries(disabledEntries, storeWarning))
        {
            for (AutorunEntry& entry : disabledEntries)
            {
                const auto existing = m_entries.find(entry.id);
                if (entry.sourceType == AutorunSourceType::Service)
                {
                    if (existing != m_entries.end() && SameSourceIdentity(existing->second, entry))
                    {
                        auto visible = std::find_if(result.entries.begin(), result.entries.end(), [&entry](const AutorunEntry& item) {
                            return item.id == entry.id && SameSourceIdentity(item, entry);
                        });
                        if (existing->second.serviceStartType == SERVICE_DISABLED)
                        {
                            existing->second.serviceOriginalStartType = entry.serviceOriginalStartType;
                            existing->second.canSetEnabled = true;
                            existing->second.readOnlyReason.clear();
                            existing->second.status = L"Disabled by WPCC (was " +
                                ServiceStartTypeLabel(entry.serviceOriginalStartType) + L")";
                            if (visible != result.entries.end()) *visible = existing->second;
                        }
                        else
                        {
                            existing->second.canSetEnabled = false;
                            existing->second.readOnlyReason = L"Stored service state does not match the current SCM configuration.";
                            existing->second.status += L" - Backup mismatch";
                            if (visible != result.entries.end()) *visible = existing->second;
                            AppendWarning(result.warning, L"A stored service backup no longer matches its current start type.");
                        }
                    }
                    else if (existing != m_entries.end())
                    {
                        m_ambiguousIds.insert(entry.id);
                    }
                    continue;
                }
                if (existing == m_entries.end())
                {
                    m_entries.emplace(entry.id, entry);
                    result.entries.push_back(std::move(entry));
                }
                else if (!SameSourceIdentity(existing->second, entry))
                {
                    m_ambiguousIds.insert(entry.id);
                }
            }
        }
        else
        {
            AppendWarning(result.warning, storeWarning);
        }

        if (!m_ambiguousIds.empty())
        {
            for (AutorunEntry& entry : result.entries)
            {
                if (m_ambiguousIds.contains(entry.id))
                {
                    entry.canSetEnabled = false;
                    entry.readOnlyReason = L"This identifier collides with another Autoruns source and cannot be changed safely.";
                }
            }
            AppendWarning(
                result.warning,
                L"One or more entries have colliding identifiers and cannot be changed safely.");
        }

        std::sort(result.entries.begin(), result.entries.end(), [](const AutorunEntry& left, const AutorunEntry& right) {
            const std::wstring leftName = ToLower(left.entryName);
            const std::wstring rightName = ToLower(right.entryName);
            if (leftName != rightName)
            {
                return leftName < rightName;
            }
            return left.id < right.id;
        });
        return result;
    }

    AutorunActionResult AutorunProvider::SetEnabled(std::string_view id, bool enabled)
    {
        const auto catalogEntry = m_entries.find(std::string(id));
        if (id.empty() || catalogEntry == m_entries.end())
        {
            return FailureResult(id, enabled, "The Autoruns entry is no longer available. Refresh and try again.");
        }
        if (m_ambiguousIds.contains(std::string(id)))
        {
            return FailureResult(
                id,
                enabled,
                "The Autoruns entry identifier is ambiguous and cannot be changed safely.");
        }

        const AutorunEntry entry = catalogEntry->second;
        if (!entry.canSetEnabled)
        {
            const std::string reason = entry.readOnlyReason.empty() ?
                "This Autoruns entry is read-only." : WideToUtf8(entry.readOnlyReason);
            return FailureResult(id, enabled, reason);
        }
        if (entry.enabled == enabled)
        {
            return SuccessResult(id, enabled, enabled ? "The Autoruns entry is already enabled." : "The Autoruns entry is already disabled.");
        }

        if (entry.sourceType == AutorunSourceType::ScheduledTask)
        {
            const HRESULT result = SetScheduledTaskEnabled(entry.taskPath, enabled);
            if (FAILED(result))
            {
                return FailureResult(
                    id, enabled, "The Scheduled Task enabled state could not be changed.", ErrorFromHresult(result));
            }
            return SuccessResult(id, enabled, enabled ? "Scheduled Task enabled." : "Scheduled Task disabled.");
        }
        if (entry.sourceType == AutorunSourceType::Driver)
        {
            return FailureResult(id, enabled, "Driver startup management is read-only in this version.");
        }

        std::vector<AutorunEntry> disabledEntries;
        std::wstring storeWarning;
        if (!LoadDisabledEntries(disabledEntries, storeWarning))
        {
            return FailureResult(id, enabled, WideToUtf8(storeWarning));
        }

        if (entry.sourceType == AutorunSourceType::Service)
        {
            const SC_HANDLE nativeManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
            const DWORD managerError = nativeManager == nullptr ? GetLastError() : ERROR_SUCCESS;
            ServiceHandle manager(nativeManager);
            if (manager.Get() == nullptr)
            {
                return FailureResult(id, enabled, "The Service Control Manager could not be opened.", managerError);
            }
            const SC_HANDLE nativeService = OpenServiceW(
                manager.Get(), entry.serviceName.c_str(), SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG);
            const DWORD serviceError = nativeService == nullptr ? GetLastError() : ERROR_SUCCESS;
            ServiceHandle service(nativeService);
            if (service.Get() == nullptr)
            {
                return FailureResult(id, enabled, "The service configuration could not be opened.", serviceError);
            }

            std::vector<unsigned char> configurationStorage;
            QUERY_SERVICE_CONFIGW* configuration = nullptr;
            const DWORD queryError = QueryServiceConfiguration(service.Get(), configurationStorage, configuration);
            if (queryError != ERROR_SUCCESS || configuration == nullptr)
            {
                return FailureResult(id, enabled, "The current service start type could not be read.", queryError);
            }

            if (!enabled)
            {
                if (configuration->dwStartType == SERVICE_DISABLED)
                {
                    return FailureResult(id, enabled, "The service is already disabled and its prior start type is unknown.");
                }

                AutorunEntry backup = entry;
                backup.enabled = false;
                backup.serviceOriginalStartType = configuration->dwStartType;
                backup.status = L"Disabled by WPCC (was " + ServiceStartTypeLabel(configuration->dwStartType) + L")";
                bool backupAdded = false;
                const auto existingBackup = FindDisabledEntry(disabledEntries, id);
                if (existingBackup != disabledEntries.end())
                {
                    if (existingBackup->sourceType != AutorunSourceType::Service ||
                        ToLower(existingBackup->serviceName) != ToLower(backup.serviceName) ||
                        existingBackup->serviceOriginalStartType != backup.serviceOriginalStartType)
                    {
                        return FailureResult(id, enabled, "A conflicting disabled backup already exists for this service.");
                    }
                }
                else
                {
                    disabledEntries.push_back(backup);
                    backupAdded = true;
                    if (!SaveDisabledEntries(disabledEntries, storeWarning))
                    {
                        return FailureResult(id, enabled, WideToUtf8(storeWarning));
                    }
                }

                if (!ChangeServiceConfigW(
                    service.Get(), SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE,
                    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
                {
                    const DWORD changeError = GetLastError();
                    if (backupAdded)
                    {
                        disabledEntries.pop_back();
                        std::wstring rollbackWarning;
                        SaveDisabledEntries(disabledEntries, rollbackWarning);
                    }
                    return FailureResult(id, enabled, "The service start type could not be disabled.", changeError);
                }
                return SuccessResult(id, false, "Service disabled; its original start type was preserved by WPCC.");
            }

            auto disabledEntry = FindDisabledEntry(disabledEntries, id);
            if (disabledEntry == disabledEntries.end() || disabledEntry->sourceType != AutorunSourceType::Service ||
                !SameSourceIdentity(*disabledEntry, entry))
            {
                return FailureResult(id, enabled, "The service's original start type backup could not be found.");
            }
            if (configuration->dwStartType != SERVICE_DISABLED)
            {
                return FailureResult(id, enabled, "Restore conflict: the service start type changed after WPCC disabled it.");
            }
            const DWORD restoreStartType = disabledEntry->serviceOriginalStartType;
            if (!ChangeServiceConfigW(
                service.Get(), SERVICE_NO_CHANGE, restoreStartType, SERVICE_NO_CHANGE,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
            {
                return FailureResult(id, enabled, "The service's original start type could not be restored.", GetLastError());
            }

            disabledEntries.erase(disabledEntry);
            if (!SaveDisabledEntries(disabledEntries, storeWarning))
            {
                const BOOL rolledBack = ChangeServiceConfigW(
                    service.Get(), SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE,
                    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
                const DWORD rollbackError = rolledBack ? 0 : GetLastError();
                std::string message = WideToUtf8(storeWarning);
                if (!rolledBack)
                {
                    message += " The restored start type could not be rolled back; its backup remains stored.";
                }
                return FailureResult(id, enabled, std::move(message), rollbackError);
            }
            return SuccessResult(id, true, "The service's original start type was restored.");
        }

        if (entry.sourceType == AutorunSourceType::RegistryValue)
        {
            const HKEY root = entry.registryCurrentUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
            RegistryKey key;
            const LSTATUS openStatus = RegOpenKeyExW(
                root, entry.registryKeyPath.c_str(), 0,
                KEY_QUERY_VALUE | KEY_SET_VALUE | entry.registryView, key.Put());
            if (openStatus != ERROR_SUCCESS)
            {
                return FailureResult(id, enabled, "The registry startup location could not be opened.", openStatus);
            }

            if (!enabled)
            {
                AutorunEntry backup = entry;
                const LSTATUS queryStatus = QueryRegistryValue(
                    key.Get(), entry.registryValueName,
                    backup.registryValueType, backup.registryValueData);
                if (queryStatus != ERROR_SUCCESS)
                {
                    return FailureResult(id, enabled, "The registry startup value could not be preserved.", queryStatus);
                }
                if (backup.registryValueType == REG_SZ || backup.registryValueType == REG_EXPAND_SZ)
                {
                    backup.command = RegistryString(backup.registryValueData);
                    const ParsedImage parsed = ParseCommandImage(backup.command);
                    backup.imagePath = parsed.path;
                }
                backup.enabled = false;
                backup.status = L"Disabled by WPCC";

                bool backupAdded = false;
                const auto existingBackup = FindDisabledEntry(disabledEntries, id);
                if (existingBackup != disabledEntries.end())
                {
                    if (existingBackup->sourceType != AutorunSourceType::RegistryValue ||
                        existingBackup->registryCurrentUser != backup.registryCurrentUser ||
                        existingBackup->registryView != backup.registryView ||
                        ToLower(existingBackup->registryKeyPath) != ToLower(backup.registryKeyPath) ||
                        existingBackup->registryValueName != backup.registryValueName ||
                        existingBackup->registryValueType != backup.registryValueType ||
                        existingBackup->registryValueData != backup.registryValueData)
                    {
                        return FailureResult(id, enabled, "A conflicting disabled backup already exists for this Autoruns entry.");
                    }
                }
                else
                {
                    disabledEntries.push_back(backup);
                    backupAdded = true;
                    if (!SaveDisabledEntries(disabledEntries, storeWarning))
                    {
                        return FailureResult(id, enabled, WideToUtf8(storeWarning));
                    }
                }

                unsigned long currentType = 0;
                std::vector<unsigned char> currentData;
                const LSTATUS recheckStatus = QueryRegistryValue(
                    key.Get(), entry.registryValueName, currentType, currentData);
                if (recheckStatus != ERROR_SUCCESS ||
                    currentType != backup.registryValueType || currentData != backup.registryValueData)
                {
                    if (backupAdded)
                    {
                        disabledEntries.pop_back();
                        std::wstring rollbackWarning;
                        SaveDisabledEntries(disabledEntries, rollbackWarning);
                    }
                    return FailureResult(
                        id,
                        enabled,
                        "The registry startup value changed or disappeared before it could be disabled.",
                        recheckStatus == ERROR_SUCCESS ? ERROR_RETRY : recheckStatus);
                }

                const LSTATUS deleteStatus = RegDeleteValueW(key.Get(), entry.registryValueName.c_str());
                if (deleteStatus != ERROR_SUCCESS)
                {
                    if (backupAdded)
                    {
                        disabledEntries.pop_back();
                        std::wstring rollbackWarning;
                        SaveDisabledEntries(disabledEntries, rollbackWarning);
                    }
                    return FailureResult(id, enabled, "The registry startup value could not be disabled.", deleteStatus);
                }
                return SuccessResult(id, false, "Autoruns entry disabled and preserved by WPCC.");
            }

            auto disabledEntry = FindDisabledEntry(disabledEntries, id);
            if (disabledEntry == disabledEntries.end())
            {
                return FailureResult(id, enabled, "The disabled registry backup could not be found.");
            }

            unsigned long existingType = 0;
            std::vector<unsigned char> existingData;
            const LSTATUS existingStatus = QueryRegistryValue(
                key.Get(), disabledEntry->registryValueName, existingType, existingData);
            if (existingStatus == ERROR_SUCCESS)
            {
                return FailureResult(id, enabled, "Restore conflict: the registry value already exists.");
            }
            if (existingStatus != ERROR_FILE_NOT_FOUND)
            {
                return FailureResult(id, enabled, "The registry restore destination could not be checked.", existingStatus);
            }

            const LSTATUS restoreStatus = RegSetValueExW(
                key.Get(), disabledEntry->registryValueName.c_str(), 0,
                disabledEntry->registryValueType,
                disabledEntry->registryValueData.empty() ? nullptr : disabledEntry->registryValueData.data(),
                static_cast<DWORD>(disabledEntry->registryValueData.size()));
            if (restoreStatus != ERROR_SUCCESS)
            {
                return FailureResult(id, enabled, "The registry startup value could not be restored.", restoreStatus);
            }

            disabledEntries.erase(disabledEntry);
            if (!SaveDisabledEntries(disabledEntries, storeWarning))
            {
                const LSTATUS rollbackStatus = RegDeleteValueW(key.Get(), entry.registryValueName.c_str());
                std::string message = WideToUtf8(storeWarning);
                if (rollbackStatus != ERROR_SUCCESS)
                {
                    message += " The restored value could not be rolled back; its backup remains in the store.";
                }
                return FailureResult(id, enabled, std::move(message), rollbackStatus);
            }
            return SuccessResult(id, true, "Autoruns entry restored.");
        }

        if (!enabled)
        {
            const std::filesystem::path disabledDirectory = DisabledFilesDirectory();
            if (disabledDirectory.empty())
            {
                return FailureResult(id, enabled, "Unable to locate the disabled Startup storage directory.");
            }
            std::error_code directoryError;
            std::filesystem::create_directories(disabledDirectory, directoryError);
            if (directoryError)
            {
                return FailureResult(id, enabled, "The disabled Startup storage directory could not be created.");
            }

            AutorunEntry backup = entry;
            const std::filesystem::path originalPath(entry.startupOriginalPath);
            const std::filesystem::path disabledPath = disabledDirectory /
                (Utf8ToWide(entry.id) + originalPath.extension().wstring());
            backup.startupDisabledPath = disabledPath.wstring();
            backup.enabled = false;
            backup.status = L"Disabled by WPCC";
            if (GetFileAttributesW(disabledPath.c_str()) != INVALID_FILE_ATTRIBUTES)
            {
                return FailureResult(id, enabled, "The disabled Startup storage destination already exists.");
            }

            bool backupAdded = false;
            const auto existingBackup = FindDisabledEntry(disabledEntries, id);
            if (existingBackup != disabledEntries.end())
            {
                if (existingBackup->sourceType != AutorunSourceType::StartupFolder ||
                    !PathsEqual(existingBackup->startupOriginalPath, backup.startupOriginalPath) ||
                    !PathsEqual(existingBackup->startupDisabledPath, backup.startupDisabledPath))
                {
                    return FailureResult(id, enabled, "A conflicting disabled backup already exists for this Startup entry.");
                }
            }
            else
            {
                disabledEntries.push_back(backup);
                backupAdded = true;
                if (!SaveDisabledEntries(disabledEntries, storeWarning))
                {
                    return FailureResult(id, enabled, WideToUtf8(storeWarning));
                }
            }

            if (!MoveFileExW(originalPath.c_str(), disabledPath.c_str(), MOVEFILE_WRITE_THROUGH))
            {
                const DWORD moveError = GetLastError();
                if (backupAdded)
                {
                    disabledEntries.pop_back();
                    std::wstring rollbackWarning;
                    SaveDisabledEntries(disabledEntries, rollbackWarning);
                }
                return FailureResult(id, enabled, "The Startup item could not be moved to disabled storage.", moveError);
            }
            return SuccessResult(id, false, "Startup entry disabled and preserved by WPCC.");
        }

        auto disabledEntry = FindDisabledEntry(disabledEntries, id);
        if (disabledEntry == disabledEntries.end())
        {
            return FailureResult(id, enabled, "The disabled Startup backup could not be found.");
        }
        const std::filesystem::path originalPath(disabledEntry->startupOriginalPath);
        const std::filesystem::path disabledPath(disabledEntry->startupDisabledPath);
        if (GetFileAttributesW(originalPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            return FailureResult(id, enabled, "Restore conflict: the original Startup destination already exists.");
        }

        if (!MoveFileExW(disabledPath.c_str(), originalPath.c_str(), MOVEFILE_WRITE_THROUGH))
        {
            return FailureResult(id, enabled, "The Startup entry could not be restored.", GetLastError());
        }

        disabledEntries.erase(disabledEntry);
        if (!SaveDisabledEntries(disabledEntries, storeWarning))
        {
            const BOOL rollbackSucceeded = MoveFileExW(
                originalPath.c_str(), disabledPath.c_str(), MOVEFILE_WRITE_THROUGH);
            const DWORD rollbackError = rollbackSucceeded ? 0 : GetLastError();
            std::string message = WideToUtf8(storeWarning);
            if (!rollbackSucceeded)
            {
                message += " The restored file could not be rolled back; its backup metadata remains in the store.";
            }
            return FailureResult(id, enabled, std::move(message), rollbackError);
        }
        return SuccessResult(id, true, "Startup entry restored.");
    }
}
