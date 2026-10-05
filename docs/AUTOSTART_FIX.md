# Autostart repair — implementation and Windows validation

Base: `471046c55a6389de7391834419e9551c455d816e` (`main`, 0.1.13).
The initial patch was reviewed and tested on Linux only. The release workflow now requires Windows Debug/Release builds, CTest, and package verification before publication. Consult the linked Actions run for actual results. **Interactive logon and reboot startup have not been validated.** Passing automated tests is not evidence of working logon startup.

## Diagnosis and scope

Confirmed in source: `Application::ApplyStartWithWindows` wrote HKCU Run, while the embedded-manifest source requests `requireAdministrator`. Errors from registry calls were discarded. `HandleSaveSettings` replied before applying startup changes. Tray creation failure could leave a hidden window. The installer selected a different build directory than the packaging script.

The Run/elevation conflict is the leading explanation of the reported failure, consistent with Microsoft's logon application guidance. The user's failure, the manifest embedded in a released EXE, and actual logon behavior were **not reproduced or inspected** in this Linux environment.

References:
- https://learn.microsoft.com/en-us/previous-versions/bb325654(v=msdn.10)
- https://learn.microsoft.com/en-us/windows/win32/taskschd/security-contexts-for-running-tasks
- https://learn.microsoft.com/en-us/windows/win32/taskschd/logon-trigger-example--c---
- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-changewindowmessagefilterex
- https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regloadkeyw

## Behavior

`StartupManager` owns only WPCC startup. The existing AutorunProvider retains its role managing other entries. A task named `WindowsProcessControlCenter.Startup.<SID>` has one SID-bound logon trigger, a ten-second delay, `InteractiveToken`, `HighestAvailable`, one EXE action, separate arguments/working directory, no idle/network/battery/runtime restriction, and IgnoreNew instance policy. There is no password or SYSTEM account. The GUI mutex still prevents duplicate instances.

Reads never re-register a missing/disabled task. Only explicit startup changes configure it. A legacy Run entry migrates automatically only if saved intent is true, no task exists, the exact previous command matches this EXE, and StartupApproved is absent or recognized as enabled. Unknown/disabled StartupApproved states are conservatively left unchanged (the value is shell-owned, not a public stable contract). The old command is removed only after verifying the task; failure is reported as a partial result. Both manual enable and disable remove the exact matching old command. Unknown commands/tasks are preserved.

Before startup read/write, the elevated process identity must match the session's Explorer identity. Standard users, alternate-admin credential elevation, or an unavailable shell get a visible error/unknown state. This deliberately does not promise unattended administrator access for a standard account. Configuration should be retried with Explorer running in the administrator's own session. Startup configuration is per user, not per installation; moving a portable copy requires explicitly toggling startup to update the action path. Ordinary in-place installer upgrades retain the same EXE path.

`SettingsService` merges settings instead of dropping unknown fields. The order is Windows configuration + verification, then atomic JSON replacement, then acknowledgement. If JSON persistence fails, the Windows change is **not rolled back**: the response explicitly reports the partial commit and actual Windows state. Old settings stay intact. If task configuration fails, JSON is not written. Changing unrelated settings never rewrites startup, even if old JSON says true. The UI serializes saves, coalesces later edits, preserves the latest explicit startup intent, rejects obsolete request IDs, and shows unknown/pending startup as an indeterminate checkbox. A successful JSON save can still carry a startup inspection warning.

Tray failure shows a usable window and retries for up to a minute. `TaskbarCreated` recreates the icon after Explorer restarts. Only this registered recovery message is added to the elevated window's UIPI filter. Closing/minimizing hides the window only while a tray icon is available. WebView startup and navigation request results, task changes, and auto-apply start are logged to `%LOCALAPPDATA%\WindowsProcessControlCenter\startup.log` (approximately 1 MiB rotation; no profile contents). A navigation request success is not a page-load confirmation.

Uninstall invokes the EXE's maintenance mode before removing files. It verifies the marker, principal, action shape, and installation path before deleting matching tasks across users. It removes only exact legacy commands from registered user profiles, including offline hives loaded temporarily with scoped backup/restore privileges. Busy/inaccessible hives or unavailable Scheduler abort uninstall, retaining the EXE; cleanup is retryable but may already have removed some owned entries. No settings/profile file is deleted. This cross-user/offline cleanup needs Windows integration testing.

The packaging scripts now feed the same Release directory to Inno Setup and stop on nonzero native-tool exit codes.

## Automated checks performed on Linux

- `g++ -std=c++20 -Wall -Wextra -Werror -Isrc tests/startup_transaction_test.cpp ...`: PASS, 10 transaction scenarios.
- `node tests/settings_queue_test.js`: PASS, 6 queue scenarios.
- `node --check web/app.js` and `web/settings-queue.js`: PASS.
- `git diff --check`: PASS.
- Transaction tests with AddressSanitizer + UndefinedBehaviorSanitizer: PASS with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer itself could not inspect `/proc` in this environment, so leak detection was not performed.
- Requested CMake configure, Debug and Release commands: attempted, blocked (`cmake: command not found`). No Windows SDK/MSVC/Inno Setup/Windows GUI is available. Installing auxiliary build tools failed due to environment permission restrictions.

The transaction tests use fake system/persistence adapters. They cover enable, disable, repeated disable, stale intent, denied/unknown configuration, disk failure after OS success, incomplete and successful migration outcomes, and unrelated settings with unavailable startup inspection. They **do not exercise COM, registry, shell, real disk-failure behavior, or the JSON merge implementation**. Queue tests cover serialized/coalesced writes, IDs, obsolete replies, explicit intent, and failure reconciliation.

## Build and package on Windows

Use an x64 VS 2022 developer PowerShell with the C++ workload, Windows SDK, CMake, Node.js for queue tests, and Inno Setup for Setup. Network is required for the pinned WebView2/nlohmann dependencies.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
cmake --build build --config Release
ctest --test-dir build -C Debug --output-on-failure
ctest --test-dir build -C Release --output-on-failure
node tests/settings_queue_test.js
.\scripts\package_release.ps1 -BuildDirectory build
.\scripts\build_installer.ps1 -BuildDirectory build
mt.exe '-inputresource:build\Release\WindowsProcessControlCenter.exe;#1' '-out:build\Release\embedded.manifest'
Select-String -Path build\Release\embedded.manifest -Pattern 'requireAdministrator'
```

Inspect the actual output manifest, not only the source. Compare SHA256 of the EXE in the Portable archive and installed Setup with `build\Release\WindowsProcessControlCenter.exe` from the final build. Version-sync in build_installer occurs before its build. Verify `web/settings-queue.js` is packaged alongside app.js.

## Windows 10 and Windows 11 integration matrix

Run these in disposable VMs with snapshots and dedicated test accounts; they intentionally alter startup configuration. Record OS build, EXE hash, account SID, task XML, timestamps and log excerpts. Do all scenarios on both OS families before declaring the fix verified.

1. Enable in GUI, wait for acknowledgement. Run `scripts\verify_startup.ps1 -Executable '<full EXE path>'`. Confirm exactly one named task and no recognized Run duplicate. Repeat enable; no duplicate task.
2. Disable; run the verifier with `-ExpectDisabled`. Repeat disable. Exit WPCC, log out/in: no process. JSON intent and switch remain off.
3. With 0.1.13 enable startup, exit, replace with patched EXE at the same path, launch manually. Confirm migration, one task and old Run removal. Repeat with StartupApproved disabled: no automatic migration. Repeat with a foreign command of the same Run value name: preserve it.
4. Disable or delete the task externally while JSON says true. Relaunch and edit an unrelated preference/favorite: no task recreation. Toggle startup explicitly to reconcile. Create a foreign task with the same name and a different marker/action: error, no overwrite/delete.
5. In a snapshot, simulate Scheduler registration denial/service unavailability. Check visible error and no false JSON success. Make the settings directory or target read-only for the test identity: enable startup and confirm a partial-write warning, truthful switch and unchanged prior settings. Restore permissions and retry. Verify unknown JSON fields and profiles.json survive.
6. Test a path such as `C:\WPCC test\Zażółć gęślą\WindowsProcessControlCenter.exe`, launch from `C:\Windows\System32`, and invoke the task. Verify web assets, settings, profiles and auto-apply. Repeat with both minimizeToTray settings. Task invocation alone is not the logon test.
7. **Actually sign out and sign back in, then separately reboot and sign in.** After the ten-second task delay, verify one process in the correct session, elevated token, GUI/tray accessibility, same settings/profile data and a real harmless auto-apply action. Check task history and startup.log from this boot. Record each result separately.
8. Repeat on battery. Confirm the process continues across power changes and is not limited to 72 hours. Start a second instance manually and via task: one process; scheduled duplicate does not leave a modal dialog.
9. Restart Explorer with WPCC elevated: icon returns. For reliable icon-failure injection, use a debugger to force Shell_NotifyIconW(NIM_ADD) to return FALSE; window stays available, close/minimize does not hide it, retries stop after a minute. Restore API behavior/restart Explorer and confirm recovery. Test unavailable WebView2 separately: visible initialization error and diagnostic log.
10. Sign in as a standard user, launch with another administrator's credentials: startup configuration must be refused with an account-context explanation. In two different administrator sessions, confirm per-SID tasks and distinct user data. Include RDP if supported by the environment.
11. Install Setup, enable startup, perform an in-place update, rerun verifier and repeat logon. Uninstall with startup enabled for two test users (one signed out): matching task/Run entries removed, foreign tasks/commands and settings/profiles remain. Simulate a busy/inaccessible offline hive: uninstall stops with files retained; retry after releasing it.

## Applying the patch

Apply from the repository root at the base commit (or review conflicts against newer main):

```powershell
git status --short
git apply --check C:\path\WPCC-autostart.patch
git apply C:\path\WPCC-autostart.patch
```

Do not use `git reset --hard` or overwrite user edits. The original patch was local. Ongoing work is on `codex/autostart-0.1.14`; CI publishes `v0.1.14-rc.1` only after all build, test and packaging gates pass. The PR remains available for review; publication does not merge it.

## Changed files

- `CMakeLists.txt`
- `docs/AUTOSTART_FIX.md`
- `installer/WindowsProcessControlCenter.iss`
- `scripts/build_installer.ps1`
- `scripts/package_release.ps1`
- `scripts/verify_startup.ps1`
- `src/app/Application.cpp`
- `src/app/Application.h`
- `src/core/SettingsService.cpp`
- `src/core/SettingsService.h`
- `src/core/SettingsStore.cpp`
- `src/core/StartupManager.cpp`
- `src/core/StartupManager.h`
- `src/core/StartupTransaction.h`
- `src/main.cpp`
- `src/ui_web/WebMessageBridge.cpp`
- `src/ui_web/WebMessageBridge.h`
- `src/ui_web/WebViewHost.cpp`
- `src/ui_web/WebViewHost.h`
- `tests/settings_queue_test.js`
- `tests/startup_transaction_test.cpp`
- `web/app.js`
- `web/index.html`
- `web/settings-queue.js`
