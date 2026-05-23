#include "ui_web/WebMessageBridge.h"

#include <Windows.h>

#include <cwchar>
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
            messageJson.find(L"\"setCpuPriority\"") != std::wstring_view::npos)
        {
            return WebMessageType::SetCpuPriority;
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

    SetGpuPreferenceRequest WebMessageBridge::ParseSetGpuPreferenceRequest(std::wstring_view messageJson) const
    {
        SetGpuPreferenceRequest request{};
        request.pid = ExtractUnsignedLong(messageJson, L"pid");
        request.expectedName = ExtractString(messageJson, L"expectedName");
        request.executablePath = ExtractString(messageJson, L"exePath");
        request.preference = ExtractString(messageJson, L"preference");
        return request;
    }

    std::wstring WebMessageBridge::BuildProcessSnapshotMessage(const std::vector<ProcessInfo>& processes) const
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

        json << L"]}";
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
