#include "core/RegistryViewReader.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>

static LSTATUS WINAPI Deny32(HKEY root, LPCWSTR path, DWORD options, REGSAM access, PHKEY key) {
    if (access & KEY_WOW64_32KEY) return ERROR_ACCESS_DENIED;
    return RegOpenKeyExW(root, path, options, access, key);
}
static LSTATUS WINAPI DenyValue(HKEY key, LPCWSTR value, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size) {
    if (std::wstring(value) == L"GlobalFlag") return ERROR_ACCESS_DENIED;
    return RegQueryValueExW(key, value, reserved, type, data, size);
}
int main() {
    const std::wstring fixture = L"Software\\WPCC.RegistryTest." + std::to_wstring(GetCurrentProcessId());
    struct Cleanup { std::wstring path; ~Cleanup() { RegDeleteTreeW(HKEY_CURRENT_USER, path.c_str()); } } cleanup{fixture};
    HKEY base = nullptr, child = nullptr;
    assert(RegCreateKeyExW(HKEY_CURRENT_USER, fixture.c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &base, nullptr) == ERROR_SUCCESS);
    RegCloseKey(base);
    auto missing = wpcc::ReadRegistryView(HKEY_CURRENT_USER, fixture + L"\\Missing", KEY_WOW64_64KEY, {L"Debugger"});
    assert(missing.missing && missing.errors.empty() && missing.values.empty());
    auto empty = wpcc::ReadRegistryView(HKEY_CURRENT_USER, fixture, KEY_WOW64_64KEY, {L"Debugger"});
    assert(!empty.missing && empty.errors.empty() && empty.values.empty());
    assert(RegCreateKeyExW(HKEY_CURRENT_USER, (fixture + L"\\example.exe").c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &child, nullptr) == ERROR_SUCCESS);
    const wchar_t debugger[] = L"C:\\WPCC test\\debugger.exe";
    assert(RegSetValueExW(child, L"Debugger", 0, REG_SZ, reinterpret_cast<const BYTE*>(debugger), sizeof(debugger)) == ERROR_SUCCESS);
    DWORD flag = 512;
    assert(RegSetValueExW(child, L"GlobalFlag", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&flag), sizeof(flag)) == ERROR_SUCCESS);
    RegCloseKey(child);
    // Reproduce the original bug on a real Windows handle, not a mocked return code.
    assert(RegOpenKeyExW(HKEY_CURRENT_USER, fixture.c_str(), 0, KEY_ENUMERATE_SUB_KEYS, &base) == ERROR_SUCCESS);
    DWORD count = 0;
    const auto oldStatus = RegQueryInfoKeyW(base, nullptr, nullptr, nullptr, &count, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    RegCloseKey(base);
    assert(oldStatus == ERROR_ACCESS_DENIED);
    for (auto view : {KEY_WOW64_64KEY, KEY_WOW64_32KEY}) {
        const auto result = wpcc::ReadRegistryView(HKEY_CURRENT_USER, fixture, view, {L"Debugger", L"GlobalFlag", L"NotConfigured"});
        assert(result.errors.empty() && result.values.size() == 2);
        assert(result.values[0].data.size() == sizeof(debugger));
    }
    wpcc::RegistryReadApi api;
    api.open = Deny32;
    const auto working = wpcc::ReadRegistryView(HKEY_CURRENT_USER, fixture, KEY_WOW64_64KEY, {L"Debugger"}, api);
    const auto denied = wpcc::ReadRegistryView(HKEY_CURRENT_USER, fixture, KEY_WOW64_32KEY, {L"Debugger"}, api);
    assert(working.values.size() == 1 && denied.values.empty() && denied.errors.size() == 1 && !denied.missing);
    assert(denied.errors[0].code == ERROR_ACCESS_DENIED && wpcc::RegistryReadErrorText(denied.errors[0]).find(L"Windows error 5") != std::wstring::npos);
    api.open = RegOpenKeyExW; api.query = DenyValue;
    const auto partial = wpcc::ReadRegistryView(HKEY_CURRENT_USER, fixture, KEY_WOW64_64KEY, {L"Debugger", L"GlobalFlag"}, api);
    assert(partial.values.size() == 1 && partial.errors.size() == 1 && partial.errors[0].operation == L"read GlobalFlag");
    // Read the real machine IFEO location through BOTH flags, without modifying it.
    for (auto view : {KEY_WOW64_64KEY, KEY_WOW64_32KEY}) {
        const auto actual = wpcc::ReadRegistryView(HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options", view, {L"Debugger", L"GlobalFlag"});
        assert(!actual.missing);
        for (const auto& error : actual.errors) {
            std::wcout << wpcc::RegistryReadErrorText(error) << L"\n";
            assert(error.operation != L"open registry location" && error.operation != L"inspect registry location");
        }
        std::cout << "Real IFEO view " << view << ": " << actual.values.size() << " values, " << actual.errors.size() << " partial read errors\n";
    }
    std::cout << "Registry scenarios passed: original permission bug, both views, empty, missing, denied view, partial value failure, real IFEO reads\n";
}
