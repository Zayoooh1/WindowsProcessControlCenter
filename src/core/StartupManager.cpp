#include "core/StartupManager.h"
#include <taskschd.h>
#include <wrl/client.h>
#include <sddl.h>
#include <ShlObj.h>
#include <shellapi.h>
#include <filesystem>
#include <vector>
#include <utility>

using Microsoft::WRL::ComPtr;
namespace
{
    constexpr auto Prefix = L"WindowsProcessControlCenter.Startup.";
    constexpr auto Marker = L"WPCC.Startup.v1:{6FDC4703-94B6-4E3D-98B1-B22588940D1E}";
    constexpr auto RunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    constexpr auto RunValue = L"WindowsProcessControlCenter";
    struct Failure { HRESULT hr; const wchar_t* stage; };
    void Check(HRESULT hr, const wchar_t* stage) { if (FAILED(hr)) throw Failure{hr, stage}; }
    void WinCheck(LSTATUS status, const wchar_t* stage) { Check(HRESULT_FROM_WIN32(status), stage); }
    struct Bstr
    {
        BSTR value = nullptr;
        Bstr() = default;
        explicit Bstr(const wchar_t* s) : value(SysAllocString(s)) { if (!value) throw Failure{E_OUTOFMEMORY, L"Allocate string"}; }
        ~Bstr() { SysFreeString(value); }
        Bstr(const Bstr&) = delete;
        operator BSTR() const { return value; }
        BSTR* out() { return &value; }
        std::wstring str() const { return value ? std::wstring(value, SysStringLen(value)) : L""; }
    };
    struct Variant
    {
        VARIANT value{};
        Variant() { VariantInit(&value); }
        explicit Variant(const std::wstring& s) : Variant() { value.vt = VT_BSTR; value.bstrVal = SysAllocString(s.c_str()); if (!value.bstrVal) throw Failure{E_OUTOFMEMORY, L"Allocate identity"}; }
        ~Variant() { VariantClear(&value); }
        Variant(const Variant&) = delete;
        operator VARIANT() const { return value; }
    };
    struct Handle { HANDLE value = nullptr; ~Handle() { if (value) CloseHandle(value); } };
    struct Key { HKEY value = nullptr; ~Key() { if (value) RegCloseKey(value); } };
    struct Privilege
    {
        Handle token;
        TOKEN_PRIVILEGES previous{};
        bool changed = false;
        explicit Privilege(const wchar_t* name)
        {
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &token.value))
                throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Open maintenance token"};
            TOKEN_PRIVILEGES desired{}; desired.PrivilegeCount = 1;
            if (!LookupPrivilegeValueW(nullptr, name, &desired.Privileges[0].Luid))
                throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Find maintenance privilege"};
            desired.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            DWORD size = sizeof(previous);
            if (!AdjustTokenPrivileges(token.value, FALSE, &desired, sizeof(previous), &previous, &size))
                throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Enable maintenance privilege"};
            const DWORD error = GetLastError();
            if (error != ERROR_SUCCESS) throw Failure{HRESULT_FROM_WIN32(error), L"Maintenance privilege is unavailable"};
            changed = true;
        }
        ~Privilege() { if (changed) AdjustTokenPrivileges(token.value, FALSE, &previous, 0, nullptr, nullptr); }
    };
    struct MountedHive
    {
        std::wstring name;
        bool loaded = false;
        ~MountedHive() { if (loaded) RegUnLoadKeyW(HKEY_USERS, name.c_str()); }
        void Close()
        {
            if (loaded) { WinCheck(RegUnLoadKeyW(HKEY_USERS, name.c_str()), L"Unload profile hive"); loaded = false; }
        }
    };
    bool Same(const std::wstring& a, const std::wstring& b) { return _wcsicmp(a.c_str(), b.c_str()) == 0; }
    bool Missing(HRESULT hr) { return hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND); }
    std::wstring TokenSid(HANDLE token)
    {
        DWORD size = 0;
        GetTokenInformation(token, TokenUser, nullptr, 0, &size);
        if (!size) throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Read token size"};
        std::vector<BYTE> bytes(size);
        if (!GetTokenInformation(token, TokenUser, bytes.data(), size, &size)) throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Read token identity"};
        LPWSTR raw = nullptr;
        if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(bytes.data())->User.Sid, &raw)) throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Read SID"};
        std::wstring sid(raw); LocalFree(raw); return sid;
    }
    std::wstring ResolveSid(const std::wstring& account)
    {
        PSID parsed = nullptr;
        if (ConvertStringSidToSidW(account.c_str(), &parsed))
        {
            LocalFree(parsed);
            return account;
        }
        DWORD sidBytes = 0, domainChars = 0; SID_NAME_USE use{};
        LookupAccountNameW(nullptr, account.c_str(), nullptr, &sidBytes, nullptr, &domainChars, &use);
        if (!sidBytes) throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Resolve task account"};
        std::vector<BYTE> sid(sidBytes); std::vector<wchar_t> domain(domainChars + 1);
        if (!LookupAccountNameW(nullptr, account.c_str(), sid.data(), &sidBytes, domain.data(), &domainChars, &use))
            throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Resolve task account"};
        LPWSTR text = nullptr;
        if (!ConvertSidToStringSidW(sid.data(), &text)) throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Format task SID"};
        std::wstring result(text); LocalFree(text); return result;
    }
    std::wstring CurrentSid(bool requireInteractive)
    {
        Handle token;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token.value)) throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Open token"};
        const auto sid = TokenSid(token.value);
        if (!requireInteractive) return sid;
        TOKEN_ELEVATION elevation{}; DWORD size = 0;
        if (!GetTokenInformation(token.value, TokenElevation, &elevation, sizeof(elevation), &size)) throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Read elevation"};
        if (!elevation.TokenIsElevated) throw Failure{E_ACCESSDENIED, L"Startup requires an elevated administrator account"};
        // Check the shell in this session, not the active console (also works in RDP).
        const HWND shell = GetShellWindow(); DWORD pid = 0;
        if (!shell || !GetWindowThreadProcessId(shell, &pid)) throw Failure{E_ACCESSDENIED, L"Cannot verify the signed-in desktop user; retry when Explorer is running"};
        Handle process; process.value = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        Handle shellToken;
        if (!process.value || !OpenProcessToken(process.value, TOKEN_QUERY, &shellToken.value)) throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Read desktop user"};
        if (sid != TokenSid(shellToken.value)) throw Failure{E_ACCESSDENIED, L"Startup is unavailable when elevated using another user's credentials; sign in as the administrator account"};
        return sid;
    }
    struct Scheduler
    {
        ComPtr<ITaskService> service;
        ComPtr<ITaskFolder> root;
        Scheduler()
        {
            Check(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&service)), L"Create Task Scheduler");
            Variant empty;
            Check(service->Connect(empty, empty, empty, empty), L"Connect Task Scheduler service");
            Check(service->GetFolder(Bstr(L"\\"), &root), L"Open task folder");
        }
        ComPtr<IRegisteredTask> Find(const std::wstring& name)
        {
            ComPtr<IRegisteredTask> task;
            const auto hr = root->GetTask(Bstr(name.c_str()), &task);
            if (!Missing(hr)) Check(hr, L"Read startup task");
            return task;
        }
    };
    std::wstring TaskName(const std::wstring& sid) { return std::wstring(Prefix) + sid; }
    ComPtr<IExecAction> OwnedAction(ITaskDefinition* def, const std::wstring& sid)
    {
        ComPtr<IRegistrationInfo> info; Check(def->get_RegistrationInfo(&info), L"Read task ownership");
        Bstr source; Check(info->get_Source(source.out()), L"Read task marker");
        ComPtr<IPrincipal> principal; Check(def->get_Principal(&principal), L"Read task principal");
        Bstr user; Check(principal->get_UserId(user.out()), L"Read task user");
        ComPtr<IActionCollection> actions; Check(def->get_Actions(&actions), L"Read actions");
        LONG count = 0; Check(actions->get_Count(&count), L"Count actions");
        if (source.str() != Marker || ResolveSid(user.str()) != sid || count != 1) throw Failure{HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), L"Task name collision: unrecognized task was left unchanged"};
        ComPtr<IAction> action; Check(actions->get_Item(1, &action), L"Read action");
        ComPtr<IExecAction> exec; Check(action.As(&exec), L"Unrecognized action was left unchanged");
        Bstr path, args;
        Check(exec->get_Path(path.out()), L"Read action path"); Check(exec->get_Arguments(args.out()), L"Read action arguments");
        if (!Same(std::filesystem::path(path.str()).filename().wstring(), L"WindowsProcessControlCenter.exe") || args.str() != L"--minimized")
            throw Failure{HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), L"Unrecognized startup action was left unchanged"};
        return exec;
    }
    bool Valid(ITaskDefinition* def, IExecAction* exec, const std::wstring& sid, const std::wstring& path)
    {
        Bstr exe, args, dir; Check(exec->get_Path(exe.out()), L"Read path"); Check(exec->get_Arguments(args.out()), L"Read arguments"); Check(exec->get_WorkingDirectory(dir.out()), L"Read working directory");
        ComPtr<IPrincipal> principal; Check(def->get_Principal(&principal), L"Read principal");
        TASK_LOGON_TYPE logon; TASK_RUNLEVEL_TYPE level;
        Check(principal->get_LogonType(&logon), L"Read logon type"); Check(principal->get_RunLevel(&level), L"Read run level");
        ComPtr<ITriggerCollection> triggers; Check(def->get_Triggers(&triggers), L"Read triggers"); LONG count = 0; Check(triggers->get_Count(&count), L"Count triggers");
        if (count != 1) return false;
        ComPtr<ITrigger> trigger; Check(triggers->get_Item(1, &trigger), L"Read trigger");
        TASK_TRIGGER_TYPE2 type; Check(trigger->get_Type(&type), L"Read trigger type"); if (type != TASK_TRIGGER_LOGON) return false;
        ComPtr<ILogonTrigger> onLogon; Check(trigger.As(&onLogon), L"Read logon trigger");
        Bstr user, end, start, delay; VARIANT_BOOL triggerEnabled;
        Check(onLogon->get_Delay(delay.out()), L"Read logon delay");
        Check(onLogon->get_UserId(user.out()), L"Read trigger user"); Check(trigger->get_Enabled(&triggerEnabled), L"Read trigger enabled");
        Check(trigger->get_StartBoundary(start.out()), L"Read trigger start"); Check(trigger->get_EndBoundary(end.out()), L"Read trigger expiry");
        ComPtr<ITaskSettings> settings; Check(def->get_Settings(&settings), L"Read task settings");
        VARIANT_BOOL battery, stopBattery, idle, network, enabled; TASK_INSTANCES_POLICY instances; Bstr limit;
        Check(settings->get_DisallowStartIfOnBatteries(&battery), L"Read battery policy"); Check(settings->get_StopIfGoingOnBatteries(&stopBattery), L"Read stop policy");
        Check(settings->get_RunOnlyIfIdle(&idle), L"Read idle policy"); Check(settings->get_RunOnlyIfNetworkAvailable(&network), L"Read network policy");
        Check(settings->get_Enabled(&enabled), L"Read enabled policy"); Check(settings->get_MultipleInstances(&instances), L"Read instances policy"); Check(settings->get_ExecutionTimeLimit(limit.out()), L"Read time limit");
        return Same(exe.str(), path) && args.str() == L"--minimized" && Same(dir.str(), std::filesystem::path(path).parent_path().wstring()) &&
            logon == TASK_LOGON_INTERACTIVE_TOKEN && level == TASK_RUNLEVEL_HIGHEST && ResolveSid(user.str()) == sid && triggerEnabled && delay.str() == L"PT10S" && start.str().empty() && end.str().empty() &&
            !battery && !stopBattery && !idle && !network && enabled && instances == TASK_INSTANCES_IGNORE_NEW && limit.str() == L"PT0S";
    }
    // Only the exact command written by the previous WPCC implementation is ours.
    bool Legacy(HKEY hive, const std::wstring& exe, bool remove)
    {
        Key key; auto status = RegOpenKeyExW(hive, RunKey, 0, KEY_QUERY_VALUE | (remove ? KEY_SET_VALUE : 0), &key.value);
        if (status == ERROR_FILE_NOT_FOUND) return false;
        WinCheck(status, L"Open legacy Run key");
        DWORD type = 0, bytes = 0; status = RegQueryValueExW(key.value, RunValue, nullptr, &type, nullptr, &bytes);
        if (status == ERROR_FILE_NOT_FOUND) return false;
        WinCheck(status, L"Read legacy startup");
        if (type != REG_SZ || bytes > 131072 || bytes % sizeof(wchar_t)) return false;
        std::vector<wchar_t> data(bytes / sizeof(wchar_t) + 1, L'\0');
        WinCheck(RegQueryValueExW(key.value, RunValue, nullptr, &type, reinterpret_cast<BYTE*>(data.data()), &bytes), L"Read legacy command");
        if (!Same(data.data(), L"\"" + exe + L"\" --minimized")) return false;
        if (remove) { status = RegDeleteValueW(key.value, RunValue); if (status != ERROR_FILE_NOT_FOUND) WinCheck(status, L"Remove legacy startup"); }
        return true;
    }
    bool LegacyAllowed()
    {
        // StartupApproved is shell-owned. Only absent or explicitly enabled entries migrate.
        BYTE data[32]{}; DWORD bytes = sizeof(data), type = 0;
        auto status = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run", RunValue, RRF_RT_REG_BINARY, &type, data, &bytes);
        if (status == ERROR_FILE_NOT_FOUND) return true;
        WinCheck(status, L"Read legacy startup approval");
        return bytes >= 4 && (data[0] == 2 || data[0] == 6);
    }
    wpcc::StartupState ReadInternal(Scheduler& scheduler, const std::wstring& sid, const std::wstring& path)
    {
        wpcc::StartupState result; result.known = true; result.legacy = Legacy(HKEY_CURRENT_USER, path, false);
        auto task = scheduler.Find(TaskName(sid));
        if (task)
        {
            result.present = true;
            ComPtr<ITaskDefinition> def; Check(task->get_Definition(&def), L"Read task definition");
            auto exec = OwnedAction(def.Get(), sid);
            VARIANT_BOOL enabled; Check(task->get_Enabled(&enabled), L"Read task enabled state");
            result.enabled = enabled && Valid(def.Get(), exec.Get(), sid, path);
            if (enabled && !result.enabled) result.warning = L"Startup task configuration differs from this installation. Toggle startup off/on to repair it.";
        }
        if (result.legacy) result.warning += L" Legacy Run startup is still present; enable or disable startup here to finish migration.";
        return result;
    }
    wpcc::StartupState Failed(const Failure& failure)
    {
        wpcc::StartupLog(failure.stage, failure.hr);
        wpcc::StartupState result; result.error = failure.hr;
        wchar_t code[32]{}; swprintf_s(code, L" (0x%08lX)", static_cast<unsigned long>(failure.hr));
        result.warning = std::wstring(failure.stage) + code;
        return result;
    }
    void Register(Scheduler& scheduler, const std::wstring& sid, const std::wstring& exe, bool exists)
    {
        ComPtr<ITaskDefinition> def; Check(scheduler.service->NewTask(0, &def), L"Create task definition");
        ComPtr<IRegistrationInfo> info; Check(def->get_RegistrationInfo(&info), L"Get registration info");
        Check(info->put_Source(Bstr(Marker)), L"Set ownership marker"); Check(info->put_Description(Bstr(L"Windows Process Control Center: interactive elevated logon startup")), L"Set description");
        ComPtr<IPrincipal> principal; Check(def->get_Principal(&principal), L"Get principal");
        Check(principal->put_UserId(Bstr(sid.c_str())), L"Set user SID"); Check(principal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN), L"Set interactive logon"); Check(principal->put_RunLevel(TASK_RUNLEVEL_HIGHEST), L"Set elevation");
        ComPtr<ITriggerCollection> triggers; Check(def->get_Triggers(&triggers), L"Get triggers");
        ComPtr<ITrigger> trigger; Check(triggers->Create(TASK_TRIGGER_LOGON, &trigger), L"Create logon trigger");
        ComPtr<ILogonTrigger> logon; Check(trigger.As(&logon), L"Get logon trigger");
        Check(logon->put_UserId(Bstr(sid.c_str())), L"Set trigger SID"); Check(logon->put_Delay(Bstr(L"PT10S")), L"Set shell delay"); Check(trigger->put_Enabled(VARIANT_TRUE), L"Enable trigger");
        ComPtr<IActionCollection> actions; Check(def->get_Actions(&actions), L"Get actions");
        ComPtr<IAction> action; Check(actions->Create(TASK_ACTION_EXEC, &action), L"Create action");
        ComPtr<IExecAction> exec; Check(action.As(&exec), L"Get exec action");
        Check(exec->put_Path(Bstr(exe.c_str())), L"Set executable"); Check(exec->put_Arguments(Bstr(L"--minimized")), L"Set arguments");
        Check(exec->put_WorkingDirectory(Bstr(std::filesystem::path(exe).parent_path().c_str())), L"Set working directory");
        ComPtr<ITaskSettings> settings; Check(def->get_Settings(&settings), L"Get task settings");
        Check(settings->put_Enabled(VARIANT_TRUE), L"Enable task");
        Check(settings->put_DisallowStartIfOnBatteries(VARIANT_FALSE), L"Allow battery startup"); Check(settings->put_StopIfGoingOnBatteries(VARIANT_FALSE), L"Allow battery operation");
        Check(settings->put_RunOnlyIfIdle(VARIANT_FALSE), L"Disable idle constraint"); Check(settings->put_RunOnlyIfNetworkAvailable(VARIANT_FALSE), L"Disable network constraint");
        Check(settings->put_ExecutionTimeLimit(Bstr(L"PT0S")), L"Remove runtime limit"); Check(settings->put_MultipleInstances(TASK_INSTANCES_IGNORE_NEW), L"Set instance policy");
        Check(settings->put_StartWhenAvailable(VARIANT_TRUE), L"Set availability policy");
        ComPtr<IRegisteredTask> registered; Variant user(sid), empty;
        // CREATE alone prevents silently replacing a task created after our ownership check.
        Check(scheduler.root->RegisterTaskDefinition(Bstr(TaskName(sid).c_str()), def.Get(), exists ? TASK_UPDATE : TASK_CREATE, user, empty, TASK_LOGON_INTERACTIVE_TOKEN, empty, &registered), L"Register startup task");
    }
}
namespace wpcc
{
    std::wstring StartupManager::ExecutablePath()
    {
        for (DWORD size = 512; size <= 32768; size *= 2)
        {
            std::wstring path(size, L'\0'); const DWORD length = GetModuleFileNameW(nullptr, path.data(), size);
            if (!length) throw Failure{HRESULT_FROM_WIN32(GetLastError()), L"Read executable path"};
            if (length < size) { path.resize(length); return path; }
        }
        throw Failure{HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), L"Executable path too long"};
    }
    StartupState StartupManager::Read()
    {
        try { const auto sid = CurrentSid(true); Scheduler scheduler; return ReadInternal(scheduler, sid, ExecutablePath()); }
        catch (const Failure& e) { return Failed(e); }
        catch (...) { return Failed({E_FAIL, L"Read startup configuration"}); }
    }
    StartupState StartupManager::SetEnabled(bool enable)
    {
        try
        {
            const auto sid = CurrentSid(true); const auto exe = ExecutablePath(); Scheduler scheduler;
            auto task = scheduler.Find(TaskName(sid));
            if (task) { ComPtr<ITaskDefinition> def; Check(task->get_Definition(&def), L"Read existing task"); OwnedAction(def.Get(), sid); }
            if (enable)
            {
                if (!Same(std::filesystem::path(exe).filename().wstring(), L"WindowsProcessControlCenter.exe"))
                    throw Failure{E_INVALIDARG, L"Restore the executable name WindowsProcessControlCenter.exe before enabling startup"};
                Register(scheduler, sid, exe, !!task);
                auto verified = ReadInternal(scheduler, sid, exe);
                if (!verified.enabled) throw Failure{E_FAIL, L"Registered task failed verification"};
            }
            else if (task) Check(task->put_Enabled(VARIANT_FALSE), L"Disable startup task");
            // The task is verified first. On cleanup failure report partial state, never success.
            Legacy(HKEY_CURRENT_USER, exe, true);
            auto result = ReadInternal(scheduler, sid, exe);
            if (result.enabled != enable || result.legacy) throw Failure{E_FAIL, L"Startup state failed verification"};
            StartupLog(enable ? L"startup.enabled" : L"startup.disabled");
            return result;
        }
        catch (const Failure& e)
        {
            auto result = Read(); auto failed = Failed(e);
            result.error = failed.error; result.warning = failed.warning + L" Configuration may have changed; the displayed state is re-read from Windows.";
            return result;
        }
        catch (...) { return Failed({E_FAIL, L"Configure startup"}); }
    }
    StartupState StartupManager::MigrateLegacy(bool savedIntent)
    {
        auto state = Read();
        if (!state.known || !state.legacy || !savedIntent || state.present) return state;
        try { if (LegacyAllowed()) return SetEnabled(true); }
        catch (const Failure& e) { return Failed(e); }
        return state;
    }
    void StartupLog(const wchar_t* stage, HRESULT result) noexcept
    {
        try
        {
            PWSTR raw = nullptr;
            if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &raw))) return;
            std::filesystem::path dir(raw); CoTaskMemFree(raw); dir /= L"WindowsProcessControlCenter";
            std::error_code ec; std::filesystem::create_directories(dir, ec);
            const auto path = dir / L"startup.log";
            if (std::filesystem::file_size(path, ec) > 1024 * 1024 && !ec) { std::filesystem::remove(dir / L"startup.previous.log", ec); std::filesystem::rename(path, dir / L"startup.previous.log", ec); }
            SYSTEMTIME now{}; GetSystemTime(&now); wchar_t line[512]{};
            swprintf_s(line, L"%04u-%02u-%02uT%02u:%02u:%02uZ pid=%lu %ls hr=0x%08lX\r\n", now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, GetCurrentProcessId(), stage, static_cast<unsigned long>(result));
            char utf8[2048]{}; const int bytes = WideCharToMultiByte(CP_UTF8, 0, line, -1, utf8, sizeof(utf8), nullptr, nullptr);
            Handle file; file.value = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file.value == INVALID_HANDLE_VALUE) { file.value = nullptr; return; }
            DWORD written = 0; if (bytes > 1) WriteFile(file.value, utf8, bytes - 1, &written, nullptr);
        }
        catch (...) {}
    }
    HRESULT StartupManager::RemoveInstallationStartup()
    {
        try
        {
            const auto exe = ExecutablePath(); Scheduler scheduler;
            ComPtr<IRegisteredTaskCollection> tasks; Check(scheduler.root->GetTasks(TASK_ENUM_HIDDEN, &tasks), L"Enumerate installation tasks");
            LONG count = 0; Check(tasks->get_Count(&count), L"Count installation tasks");
            for (LONG i = count; i >= 1; --i)
            {
                VARIANT index{}; index.vt = VT_I4; index.lVal = i;
                ComPtr<IRegisteredTask> task; Check(tasks->get_Item(index, &task), L"Read installation task");
                Bstr name; Check(task->get_Name(name.out()), L"Read task name"); auto text = name.str();
                if (text.rfind(Prefix, 0) != 0) continue;
                ComPtr<ITaskDefinition> def; Check(task->get_Definition(&def), L"Read installation definition");
                ComPtr<IExecAction> exec;
                try { exec = OwnedAction(def.Get(), text.substr(wcslen(Prefix))); }
                catch (const Failure& e) { if (e.hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) || e.hr == E_NOINTERFACE) continue; throw; }
                Bstr path; Check(exec->get_Path(path.out()), L"Read installed executable");
                if (Same(path.str(), exe)) Check(scheduler.root->DeleteTask(name, 0), L"Delete installation task");
            }
            // Inspect all registered profiles, including offline users, without changing their settings files.
            Key profiles; WinCheck(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList", 0, KEY_READ, &profiles.value), L"Open user profiles");
            for (DWORD i = 0;; ++i)
            {
                wchar_t sid[256]{}; DWORD chars = 256;
                auto status = RegEnumKeyExW(profiles.value, i, sid, &chars, nullptr, nullptr, nullptr, nullptr);
                if (status == ERROR_NO_MORE_ITEMS) break;
                WinCheck(status, L"Enumerate user profiles");
                // Normal domain/local and Azure AD user SIDs only; skip service profiles.
                if (wcsncmp(sid, L"S-1-5-21-", 9) && wcsncmp(sid, L"S-1-12-1-", 9)) continue;
                Key hive; status = RegOpenKeyExW(HKEY_USERS, sid, 0, KEY_READ | KEY_WRITE, &hive.value);
                if (status == ERROR_FILE_NOT_FOUND)
                {
                    wchar_t folder[32768]{}; DWORD bytes = sizeof(folder);
                    WinCheck(RegGetValueW(profiles.value, sid, L"ProfileImagePath", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, nullptr, folder, &bytes), L"Read profile path");
                    wchar_t expanded[32768]{};
                    const DWORD length = ExpandEnvironmentStringsW(folder, expanded, 32768);
                    if (!length || length > 32768) throw Failure{HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), L"Expand profile path"};
                    const auto ntuser = std::filesystem::path(expanded) / L"NTUSER.DAT";
                    const auto attributes = GetFileAttributesW(ntuser.c_str());
                    if (attributes == INVALID_FILE_ATTRIBUTES)
                    {
                        const auto error = GetLastError();
                        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) continue;
                        WinCheck(error, L"Inspect offline profile hive");
                    }
                    Privilege backup(SE_BACKUP_NAME), restore(SE_RESTORE_NAME);
                    MountedHive mount;
                    mount.name = L"WPCC.Uninstall." + std::to_wstring(GetCurrentProcessId()) + L"." + sid;
                    Key existing;
                    const auto found = RegOpenKeyExW(HKEY_USERS, mount.name.c_str(), 0, KEY_READ, &existing.value);
                    if (found != ERROR_FILE_NOT_FOUND) throw Failure{HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), L"Temporary hive name is already in use"};
                    WinCheck(RegLoadKeyW(HKEY_USERS, mount.name.c_str(), ntuser.c_str()), L"Load offline profile hive");
                    mount.loaded = true;
                    // Nested scope ensures all registry handles close before unloading, also on failure.
                    {
                        Key offline;
                        WinCheck(RegOpenKeyExW(HKEY_USERS, mount.name.c_str(), 0, KEY_READ | KEY_WRITE, &offline.value), L"Open offline profile hive");
                        Legacy(offline.value, exe, true);
                    }
                    mount.Close();
                    continue;
                }
                WinCheck(status, L"Open profile hive for startup cleanup");
                Legacy(hive.value, exe, true);
            }
            StartupLog(L"startup.uninstall-cleanup"); return S_OK;
        }
        catch (const Failure& e) { Failed(e); return e.hr; }
        catch (...) { StartupLog(L"startup.uninstall-cleanup", E_FAIL); return E_FAIL; }
    }
}
