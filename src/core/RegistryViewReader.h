#pragma once
#include <Windows.h>
#include <algorithm>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace wpcc {
struct RegistryValueRecord {
    std::wstring keyPath, target, name;
    DWORD type = 0;
    std::vector<unsigned char> data;
};
struct RegistryReadError { LSTATUS code; std::wstring operation, path; };
struct RegistryViewResult {
    bool missing = false;
    std::vector<RegistryValueRecord> values;
    std::vector<RegistryReadError> errors;
};
// API injection is used by Windows tests to exercise inaccessible views and partial reads.
struct RegistryReadApi {
    decltype(&RegOpenKeyExW) open = &RegOpenKeyExW;
    decltype(&RegQueryInfoKeyW) info = &RegQueryInfoKeyW;
    decltype(&RegEnumKeyExW) enumerate = &RegEnumKeyExW;
    decltype(&RegQueryValueExW) query = &RegQueryValueExW;
};
struct ReadKey {
    HKEY value = nullptr;
    ~ReadKey() { if (value) RegCloseKey(value); }
};
inline bool RegistryMissing(LSTATUS code) { return code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND; }
inline RegistryViewResult ReadRegistryView(HKEY root, const std::wstring& path, REGSAM view,
    std::initializer_list<std::wstring_view> names, const RegistryReadApi& api = {}) {
    RegistryViewResult result;
    ReadKey base;
    // RegQueryInfoKey requires KEY_QUERY_VALUE even when only subkey information is requested.
    auto status = api.open(root, path.c_str(), 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | view, &base.value);
    if (RegistryMissing(status)) { result.missing = true; return result; }
    if (status != ERROR_SUCCESS) { result.errors.push_back({status, L"open registry location", path}); return result; }
    DWORD count = 0, maxName = 0;
    status = api.info(base.value, nullptr, nullptr, nullptr, &count, &maxName, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (status != ERROR_SUCCESS) { result.errors.push_back({status, L"inspect registry location", path}); return result; }
    for (DWORD i = 0; i < count; ++i) {
        std::wstring target(static_cast<size_t>(maxName) + 1, L'\0');
        DWORD length = static_cast<DWORD>(target.size());
        status = api.enumerate(base.value, i, target.data(), &length, nullptr, nullptr, nullptr, nullptr);
        if (status == ERROR_NO_MORE_ITEMS) break;
        if (status != ERROR_SUCCESS) { result.errors.push_back({status, L"enumerate registry location", path}); continue; }
        target.resize(length);
        const auto childPath = path + L"\\" + target;
        ReadKey child;
        status = api.open(root, childPath.c_str(), 0, KEY_QUERY_VALUE | view, &child.value);
        if (RegistryMissing(status)) continue; // The key may disappear during enumeration.
        if (status != ERROR_SUCCESS) { result.errors.push_back({status, L"read program options", childPath}); continue; }
        for (auto name : names) {
            RegistryValueRecord value{childPath, target, std::wstring(name), 0, {}};
            DWORD bytes = 0;
            status = api.query(child.value, value.name.c_str(), nullptr, &value.type, nullptr, &bytes);
            if (RegistryMissing(status)) continue; // An absent optional value is normal.
            if (status == ERROR_SUCCESS) {
                for (int attempt = 0; attempt < 3; ++attempt) {
                    if (bytes > 16 * 1024 * 1024) { status = ERROR_MORE_DATA; break; }
                    value.data.resize(std::max<DWORD>(bytes, 1));
                    DWORD capacity = static_cast<DWORD>(value.data.size());
                    status = api.query(child.value, value.name.c_str(), nullptr, &value.type, value.data.data(), &capacity);
                    bytes = capacity;
                    if (status != ERROR_MORE_DATA) { if (status == ERROR_SUCCESS) value.data.resize(bytes); break; }
                }
            }
            if (RegistryMissing(status)) continue;
            if (status != ERROR_SUCCESS) result.errors.push_back({status, L"read " + value.name, childPath});
            else result.values.push_back(std::move(value));
        }
    }
    return result;
}
inline std::wstring RegistryReadErrorText(const RegistryReadError& error) {
    wchar_t message[512]{};
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
        static_cast<DWORD>(error.code), 0, message, 512, nullptr);
    std::wstring text = error.code == ERROR_ACCESS_DENIED
        ? L"Access denied. This location's permissions do not allow reading it."
        : (message[0] ? std::wstring(message) : L"Windows could not read this location.");
    while (!text.empty() && (text.back() == L'\r' || text.back() == L'\n')) text.pop_back();
    return L"Could not " + error.operation + L": " + text + L" (Windows error " + std::to_wstring(error.code) + L"; " + error.path + L"). Available results are still shown.";
}
}
