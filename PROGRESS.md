# Progress

## Task 01 - Project Foundation

Status: completed.

Created a clean C++20 Win32 desktop application foundation named `WindowsProcessControlCenter`. The app uses DirectX 11 and Dear ImGui for a modern dark UI shell. Real Windows process management is intentionally out of scope for this task.

## Added and Changed Files

- `CMakeLists.txt`
- `README.md`
- `PROGRESS.md`
- `docs/.gitkeep`
- `third_party/.gitkeep`
- `src/main.cpp`
- `src/app/Application.h`
- `src/app/Application.cpp`
- `src/core/ProcessInfo.h`
- `src/core/ProcessMockProvider.h`
- `src/core/ProcessMockProvider.cpp`
- `src/platform/D3D11Renderer.h`
- `src/platform/D3D11Renderer.cpp`
- `src/platform/Win32Window.h`
- `src/platform/Win32Window.cpp`
- `src/ui/UiLayer.h`
- `src/ui/UiLayer.cpp`

## What Works

- CMake project generation for Visual Studio/MSVC.
- C++20 Windows desktop target.
- Dear ImGui dependency download via CMake `FetchContent`.
- Win32 window creation with a minimum size.
- DirectX 11 device, swap chain, render target, resize handling, and present loop.
- Dear ImGui frame lifecycle.
- Modern dark UI shell with top bar, sidebar, process table, search field, and details panel.
- Mock process data displayed in the table.
- Placeholder process action buttons with `Not implemented yet` feedback.

## Build Verification

- `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
- `cmake --build build --config Debug`
- `cmake --build build --config Release`
- Application launch smoke test passed.
- Layout screenshots captured at `1280x720`, `1366x768`, `1600x900`, and `1920x1080`.

Generated executables:

- `C:\Vibe\WinProcessManager\build\Debug\WindowsProcessControlCenter.exe`
- `C:\Vibe\WinProcessManager\build\Release\WindowsProcessControlCenter.exe`

## Not Implemented Yet

- Real process enumeration.
- Ending processes.
- Freezing or resuming processes.
- Changing CPU priority.
- Changing GPU preference.
- Settings persistence.
- Profiles and rules.
- Autostart behavior.
- Elevation or admin-mode workflow.

## Suggested Next Stage

Implement safe read-only process enumeration in `src/core/`, then replace the mock provider with a real process snapshot provider while keeping all destructive process actions disabled.

## Task 02 - Read-Only Windows Process Enumeration

Status: completed.

Replaced the mock process rows with a real read-only process snapshot provider. The application now enumerates active Windows processes, attempts to read executable paths and CPU priority classes, and reports access status without crashing when a process cannot be opened.

## Added, Changed, and Removed Files

- Changed `CMakeLists.txt`
- Changed `README.md`
- Changed `PROGRESS.md`
- Changed `src/core/ProcessInfo.h`
- Added `src/core/ProcessProvider.h`
- Added `src/core/ProcessProvider.cpp`
- Changed `src/ui/UiLayer.h`
- Changed `src/ui/UiLayer.cpp`
- Removed `src/core/ProcessMockProvider.h`
- Removed `src/core/ProcessMockProvider.cpp`

## What Works

- Real process enumeration through `CreateToolhelp32Snapshot`, `Process32FirstW`, and `Process32NextW`.
- Safe process probing through `OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION)`.
- Best-effort executable path lookup with `QueryFullProcessImageNameW`.
- CPU priority class lookup with `GetPriorityClass`.
- Proper handle cleanup through RAII.
- Processes that cannot be opened remain visible and are labeled as `Access denied`, `Protected/System`, or `Unknown`.
- Refresh button reloads the process snapshot on demand.
- Search filters by process name, PID, and executable path.
- Details panel shows process name, PID, executable path, CPU priority, access status, admin requirement hint, and access error details when available.

## Known Limitations

- Some protected, system, or cross-integrity processes may not expose full path or priority details from a normal user session.
- Access classification is conservative and based on WinAPI result codes plus a small system-process heuristic.
- Process data is a snapshot refreshed at startup or when `Refresh` is clicked, not a live stream.
- CPU usage, memory usage, process tree, signatures, icons, and command lines are not shown yet.

## Still Not Implemented

- Ending processes.
- Freezing or resuming processes.
- Changing CPU priority.
- Changing GPU preference.
- Settings persistence.
- Profiles and rules.
- Autostart behavior.
- Elevation or admin-mode workflow.

## Build Verification

- `cmake --build build --config Debug`
- `cmake --build build --config Release`

Generated executables:

- `C:\Vibe\WinProcessManager\build\Debug\WindowsProcessControlCenter.exe`
- `C:\Vibe\WinProcessManager\build\Release\WindowsProcessControlCenter.exe`

## Suggested Next Stage

Add richer read-only process metadata such as memory usage, CPU usage snapshot, parent PID, process architecture, icons, and command-line details before enabling any management actions.

## Task 03 - Modern UI / UX Overhaul

Status: completed.

Refined the application UI without adding new system capabilities. The app still uses the Task 02 read-only process enumeration path, but the presentation now has a more deliberate desktop-tool feel with a dedicated theme module, Segoe UI font loading, calmer colors, better spacing, badges, and clearer panel hierarchy.

## Added and Changed Files

- Changed `CMakeLists.txt`
- Changed `README.md`
- Changed `PROGRESS.md`
- Changed `src/ui/UiLayer.h`
- Changed `src/ui/UiLayer.cpp`
- Added `src/ui/UiTheme.h`
- Added `src/ui/UiTheme.cpp`

## UI Improvements

- Added a central `UiTheme` module for palette, ImGui styling, and system font loading.
- Switched to Segoe UI from `C:\Windows\Fonts\segoeui.ttf` with ImGui default font fallback.
- Reworked the dark theme with softer panels, subtle borders, restrained accent colors, and less stock ImGui styling.
- Redesigned the top bar with app title, subtitle, user mode badge, process counter, and refresh action.
- Improved sidebar spacing, active item treatment, and visual separation.
- Redesigned the process table with clearer column sizing, row spacing, selected-row treatment, path truncation, and tooltips for long text.
- Added badge/chip rendering for CPU priority, admin hint, and access status.
- Improved the right details panel with sections for basic information, executable path, runtime status, and future actions.
- Changed future process actions to disabled buttons with `Not implemented yet` tooltips.

## Still Not Implemented

- Ending processes.
- Freezing or resuming processes.
- Changing CPU priority.
- Changing GPU preference.
- Settings persistence.
- Profiles and rules.
- Autostart behavior.
- Registry modifications.
- Forced administrator elevation.

## Build Verification

- `cmake --build build --config Debug`
- `cmake --build build --config Release`

Generated executables:

- `C:\Vibe\WinProcessManager\build\Debug\WindowsProcessControlCenter.exe`
- `C:\Vibe\WinProcessManager\build\Release\WindowsProcessControlCenter.exe`

## Suggested Next Stage

Add richer read-only process metadata and sorting controls, such as memory usage, CPU usage sampling, parent PID, architecture, icons, and stable table sorting, before enabling any process management actions.

## Task 04 - UI/UX Polish v2, Layout Fixes, and GUI Library Decision

Status: completed.

Continued UI-only polish without changing process-management behavior. The layout now has safer global margins, more consistent panel padding, a less stock ImGui table, clearer sidebar spacing, and a documented GUI library decision.

## Added and Changed Files

- Changed `README.md`
- Changed `PROGRESS.md`
- Added `docs/GUI_LIBRARY_DECISION.md`
- Changed `src/ui/UiLayer.cpp`
- Changed `src/ui/UiTheme.h`
- Changed `src/ui/UiTheme.cpp`

## UI Improvements

- Added centralized layout metrics in `UiTheme` for app padding, panel padding, gaps, row height, button height, and panel sizing.
- Increased Segoe UI base font size and applied DPI-aware ImGui style scaling.
- Shifted the whole application inward so no major panel starts at the window edge.
- Refined the dark palette toward graphite panels, softer borders, and calmer accent colors.
- Improved top bar spacing, right-side control alignment, process counter placement, and status badge styling.
- Improved sidebar padding, active indicator, item height, hover state, and spacing below `NAVIGATION`.
- Improved the process table with wider PID padding, taller rows, less aggressive selection, hover treatment, wider badge columns, and better path tooltip behavior.
- Improved search input styling with stronger padding, rounded frame, and calmer border/background.
- Reworked the details panel into clearer sections: Basic information, Executable path, Runtime status, CPU priority, Access status, Admin requirement, and Future actions.
- Kept future actions visibly disabled instead of presenting them as active blue buttons.

## Layout Fixes

- Fixed UI feeling attached to the top, left, and right window edges.
- Fixed sidebar text appearing too close to the left edge.
- Fixed the PID column feeling too close to the table edge.
- Reduced cramped spacing between detail sections.
- Reduced stock ImGui table appearance through row height, padding, colors, badges, and hover/selection treatment.
- Preserved tooltip behavior for long executable paths.

## GUI Library Decision Summary

The recommendation is to stay on Dear ImGui for now. It keeps the build simple, fits the existing Win32/DX11/C++ codebase, and is efficient for dense process-manager workflows. Migration should be revisited later if accessibility, native Windows polish, or complex multi-window UI becomes more important. If migration becomes necessary, WinUI 3 is the strongest native Windows option, while Qt Widgets is a safer mature desktop alternative.

## Still Not Implemented

- Ending processes.
- Freezing or resuming processes.
- Changing CPU priority.
- Changing GPU preference.
- Settings persistence.
- Profiles and rules.
- Autostart behavior.
- Registry modifications.
- Forced administrator elevation.

## Build Verification

- `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
- `cmake --build build --config Debug`
- `cmake --build build --config Release`

Generated executables:

- `C:\Vibe\WinProcessManager\build\Debug\WindowsProcessControlCenter.exe`
- `C:\Vibe\WinProcessManager\build\Release\WindowsProcessControlCenter.exe`

## Suggested Next Stage

Add read-only table sorting and richer process metadata, then perform visual QA with screenshots before starting any safe process-action design.

## Task 05 - Refactor UI from Dear ImGui to WebView2

Status: completed.

Migrated the active UI from Dear ImGui/DX11 to Microsoft Edge WebView2. The application remains a native Win32 C++ desktop app, and the existing `ProcessProvider` backend remains responsible for read-only Windows process enumeration. The UI is now a local vanilla HTML/CSS/JavaScript frontend loaded from the copied `web/` folder.

## Added Files

- `src/ui_web/WebViewHost.h`
- `src/ui_web/WebViewHost.cpp`
- `src/ui_web/WebMessageBridge.h`
- `src/ui_web/WebMessageBridge.cpp`
- `web/index.html`
- `web/styles.css`
- `web/app.js`

## Changed Files

- `CMakeLists.txt`
- `README.md`
- `PROGRESS.md`
- `docs/GUI_LIBRARY_DECISION.md`
- `src/app/Application.h`
- `src/app/Application.cpp`

## Removed From Active Build

- Dear ImGui FetchContent dependency.
- ImGui backend/source compilation.
- `src/platform/D3D11Renderer.cpp`
- `src/platform/D3D11Renderer.h`
- `src/ui/UiLayer.cpp`
- `src/ui/UiLayer.h`
- `src/ui/UiTheme.cpp`
- `src/ui/UiTheme.h`

The files remain in the repository as legacy reference, but WebView2 is now the normal startup path.

## What Works

- CMake automatically downloads `Microsoft.Web.WebView2` version `1.0.3967.48` from NuGet.
- WebView2 SDK is extracted under `build/_deps/`.
- WebView2 loader is linked statically.
- `web/index.html`, `web/styles.css`, and `web/app.js` are copied beside the `.exe` after build.
- App starts as a native Win32 window and loads local WebView2 frontend files.
- Frontend sends `{ "type": "refreshProcesses" }` to C++.
- C++ reads real process data through `ProcessProvider`.
- C++ sends `{ "type": "processSnapshot", "processes": [...] }` to JavaScript.
- Frontend renders process table, search/filtering, selection, details panel, badges, and disabled future actions.
- Refresh button requests a fresh backend snapshot.

## Known Limitations

- WebView2 Runtime must be installed on the target machine.
- The frontend is intentionally vanilla HTML/CSS/JS; there is no bundler, framework, or component compiler.
- Table sorting is not implemented yet.
- The old ImGui files have not been deleted yet.
- Release disables WebView2 dev tools, but Debug leaves them available for development.

## Still Not Implemented

- Ending processes.
- Freezing or resuming processes.
- Changing CPU priority.
- Changing GPU preference.
- Settings persistence.
- Profiles and rules.
- Autostart behavior.
- Registry modifications.
- Forced administrator elevation.

## Build Verification

- `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
- `cmake --build build --config Debug`
- `cmake --build build --config Release`
- Application launch smoke test passed.
- WebView2 loaded local frontend from `build/Debug/web/index.html`.
- Process snapshot loaded in the WebView2 UI.
- Refresh, search, row selection, and details panel were smoke-tested.

Generated executables:

- `C:\Vibe\WinProcessManager\build\Debug\WindowsProcessControlCenter.exe`
- `C:\Vibe\WinProcessManager\build\Release\WindowsProcessControlCenter.exe`

## Suggested Next Stage

Add frontend table sorting, column sizing persistence, and richer read-only process metadata while keeping all process actions disabled.

## Task 06 - CPU Priority Control Through WebView2 UI

Status: completed.

Added the first real process-management action: changing CPU priority for an accessible selected process. The action is implemented in a dedicated backend module and exposed through the WebView2 message bridge. Other process actions remain disabled.

## Added Files

- `src/core/ProcessActions.h`
- `src/core/ProcessActions.cpp`

## Changed Files

- `CMakeLists.txt`
- `README.md`
- `PROGRESS.md`
- `src/ui_web/WebMessageBridge.h`
- `src/ui_web/WebMessageBridge.cpp`
- `src/ui_web/WebViewHost.h`
- `src/ui_web/WebViewHost.cpp`
- `web/app.js`
- `web/styles.css`

## What Works

- WebView2 details panel now includes an active CPU Priority section.
- Supported priority levels: `Realtime`, `High`, `Above Normal`, `Normal`, `Below Normal`, `Idle`.
- Frontend sends `{ "type": "setCpuPriority", "pid": ..., "priority": "...", "confirmRealtime": ... }`.
- Backend applies the action through `ProcessActions::SetCpuPriority`.
- Backend responds with `actionResult`.
- On success, the backend sends a fresh process snapshot so the UI reflects the new priority.
- UI keeps the selected process when it still exists after refresh.

## Safety Guards

- Blocks PID 0.
- Blocks the WindowsProcessControlCenter process itself.
- Blocks `System` and `Protected/System` processes.
- Blocks `Access denied` and inaccessible processes.
- Detects processes that exited between snapshot and action.
- Uses minimal required process rights: `PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION`.
- Realtime priority requires a frontend checkbox and backend `confirmRealtime == true`.

## Test Notes

- Changed a Notepad process from `Normal` to `BelowNormal` through the WebView2 UI.
- Verified the change with PowerShell `Get-Process`.
- Restored the test Notepad process to `Normal` after verification.
- Verified the Realtime UI warning and disabled Apply button without confirmation.

## Still Not Implemented

- Ending processes.
- Freezing or resuming processes.
- Changing GPU preference.
- Settings persistence.
- Profiles and rules.
- Autostart behavior.
- Registry modifications.
- Forced administrator elevation.

## Build Verification

- `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
- `cmake --build build --config Debug`
- `cmake --build build --config Release`
- Application launch smoke test passed.
- WebView2 UI loaded.
- Process list and Refresh still work.
- Search and process selection still work.

Generated executables:

- `C:\Vibe\WinProcessManager\build\Debug\WindowsProcessControlCenter.exe`
- `C:\Vibe\WinProcessManager\build\Release\WindowsProcessControlCenter.exe`

## Suggested Next Stage

Add non-destructive UX polish around CPU priority changes, such as clearer permission explanations and table sorting, before implementing any destructive actions like End Process.

## Task 07 - Safe End Process Action Through WebView2 UI

Status: completed.

Added the second real process-management action: safely ending a selected process. The action is implemented in `ProcessActions`, exposed through the WebView2 message bridge, and guarded by a custom confirmation modal in the frontend. Freeze, resume, GPU preference, profiles, and persistence remain out of scope.

## Added Files

- None.

## Changed Files

- `README.md`
- `PROGRESS.md`
- `src/core/ProcessActions.h`
- `src/core/ProcessActions.cpp`
- `src/ui_web/WebMessageBridge.h`
- `src/ui_web/WebMessageBridge.cpp`
- `src/ui_web/WebViewHost.h`
- `src/ui_web/WebViewHost.cpp`
- `web/index.html`
- `web/styles.css`
- `web/app.js`

## What Works

- Details panel now includes an active `End Process` action for eligible user processes.
- Frontend opens a dark-theme confirmation modal instead of using browser `alert()` or `confirm()`.
- Confirmation requires the exact process name, or PID when the process name is unavailable.
- Frontend sends `{ "type": "terminateProcess", "pid": ..., "expectedName": "...", "confirmation": "..." }`.
- Backend validates the request through `ProcessActions::TerminateProcessByPid`.
- Backend responds with an `actionResult` message.
- On success, the backend sends a fresh process snapshot so the terminated process disappears from the table/details.

## Safety Guards

- Blocks PID 0 and PID 4.
- Blocks the WindowsProcessControlCenter process itself.
- Blocks `Protected/System`, `Access denied`, and inaccessible processes.
- Blocks critical Windows process names: `System`, `Registry`, `smss.exe`, `csrss.exe`, `wininit.exe`, `winlogon.exe`, `services.exe`, `lsass.exe`, `svchost.exe`, `fontdrvhost.exe`, `dwm.exe`, and `explorer.exe`.
- Verifies the expected process name against the current snapshot to reduce stale-selection risk.
- Detects processes that exited before the action and reports `Process is no longer running.`
- Uses limited process rights: `PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE`, with `SYNCHRONIZE` used only to briefly wait for termination before refreshing the UI snapshot.
- Does not terminate child processes or process trees.

## Still Not Implemented

- Freezing or resuming processes.
- Changing GPU preference.
- Process tree termination.
- Force-killing child processes.
- Settings persistence.
- Profiles and rules.
- Autostart behavior.
- Registry modifications.
- Forced administrator elevation.

## Build Verification

- `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
- `cmake --build build --config Debug`
- `cmake --build build --config Release`

Generated executables:

- `C:\Vibe\WinProcessManager\build\Debug\WindowsProcessControlCenter.exe`
- `C:\Vibe\WinProcessManager\build\Release\WindowsProcessControlCenter.exe`

## Suggested Next Stage

Add safer UX around destructive actions, such as action history and clearer post-action refresh state, or implement a non-destructive Freeze/Resume design review before adding more process controls.

## Task 08 - Safe Freeze and Resume Process Actions Through WebView2 UI

Status: completed.

Added safe Freeze and Resume controls for single selected user processes in the current WindowsProcessControlCenter session. The implementation uses documented thread-level WinAPI calls and keeps session-local state so Resume only affects threads suspended by this application.

## Added Files

- None.

## Changed Files

- `README.md`
- `PROGRESS.md`
- `src/core/ProcessInfo.h`
- `src/core/ProcessActions.h`
- `src/core/ProcessActions.cpp`
- `src/ui_web/WebMessageBridge.h`
- `src/ui_web/WebMessageBridge.cpp`
- `src/ui_web/WebViewHost.h`
- `src/ui_web/WebViewHost.cpp`
- `web/index.html`
- `web/styles.css`
- `web/app.js`

## What Works

- Process snapshots now include `isFrozenByApp`.
- WebView2 process table shows runtime state as `Running` or `Frozen by app`.
- Details panel shows runtime status.
- Freeze button opens a custom confirmation modal requiring exact process name or PID.
- Frontend sends `{ "type": "freezeProcess", "pid": ..., "expectedName": "...", "confirmation": "..." }`.
- Backend suspends available threads through `CreateToolhelp32Snapshot`, `Thread32First`, `Thread32Next`, `OpenThread`, and `SuspendThread`.
- Backend records the thread IDs and resume counts frozen by this app.
- Resume button is active only for processes frozen by this app.
- Frontend sends `{ "type": "resumeProcess", "pid": ..., "expectedName": "..." }`.
- Backend resumes only the recorded threads through `ResumeThread`.
- After Freeze or Resume, the UI receives a fresh process snapshot.
- On shutdown, WebViewHost calls `ResumeAllFrozenProcesses()` to best-effort resume processes frozen by this app during the session.

## Safety Guards

- Blocks PID 0 and PID 4.
- Blocks the WindowsProcessControlCenter process itself.
- Blocks `Protected/System`, `Access denied`, and inaccessible processes.
- Blocks critical Windows process names: `System`, `Registry`, `smss.exe`, `csrss.exe`, `wininit.exe`, `winlogon.exe`, `services.exe`, `lsass.exe`, `svchost.exe`, `fontdrvhost.exe`, `dwm.exe`, `explorer.exe`, and `audiodg.exe`.
- Verifies the expected process name against the current snapshot before Freeze/Resume.
- Detects processes that exited before action and reports `Process is no longer running.`
- Does not use `NtSuspendProcess` or `NtResumeProcess`.
- Does not run `ResumeThread` in a loop to zero; it only resumes counts created by this app.
- Does not freeze or resume child processes or process trees.

## Test Notes

- Froze a test `cmd.exe` process through WebView2 UI; table and details showed `Frozen by app`.
- Resumed the same `cmd.exe`; table and details returned to `Running`.
- Closed WindowsProcessControlCenter after freezing a test process; shutdown path ran without crashing and the test process remained recoverable.
- Verified Debug and Release builds after the implementation.

## Still Not Implemented

- Changing GPU preference.
- Profiles and rules.
- Autostart behavior.
- Settings persistence.
- Registry modifications.
- Forced administrator elevation.
- Freeze/resume of process trees.
- Restart Explorer.
- Persistent frozen-process state across app restarts.

## Build Verification

- `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
- `cmake --build build --config Debug`
- `cmake --build build --config Release`

Generated executables:

- `C:\Vibe\WinProcessManager\build\Debug\WindowsProcessControlCenter.exe`
- `C:\Vibe\WinProcessManager\build\Release\WindowsProcessControlCenter.exe`

## Suggested Next Stage

Add GPU Preference design and capability research, or add an action history/log panel so users can review CPU priority, End Process, Freeze, and Resume outcomes in one place.

## Task 09 - GPU Preference Per Executable Through WebView2 UI

Status: completed.

Added Windows GPU Preference management for the selected process executable path. This setting is stored per user and per executable path through Windows Graphics Settings registry values, and it may require restarting the target app to take effect.

## Added Files

- `src/core/GpuPreferenceManager.h`
- `src/core/GpuPreferenceManager.cpp`

## Changed Files

- `CMakeLists.txt`
- `README.md`
- `PROGRESS.md`
- `src/core/ProcessActions.h`
- `src/core/ProcessInfo.h`
- `src/ui_web/WebMessageBridge.h`
- `src/ui_web/WebMessageBridge.cpp`
- `src/ui_web/WebViewHost.h`
- `src/ui_web/WebViewHost.cpp`
- `web/index.html`
- `web/styles.css`
- `web/app.js`

## What Works

- Process snapshots now include `gpuPreference`.
- WebView2 process table shows `GPU Preference`.
- Details panel includes an active GPU Preference section.
- Supported values: `SystemDefault`, `PowerSaving`, and `HighPerformance`.
- `SystemDefault` removes the executable path value from `HKCU\Software\Microsoft\DirectX\UserGpuPreferences`.
- `PowerSaving` writes `GpuPreference=1;`.
- `HighPerformance` writes `GpuPreference=2;`.
- Frontend sends `{ "type": "setGpuPreference", "pid": ..., "expectedName": "...", "exePath": "...", "preference": "..." }`.
- Backend validates the process identity and executable path before writing HKCU.
- Backend returns `actionResult` with `currentPreference`.
- Successful updates trigger a fresh process snapshot.

## GPU Preference Limitations

- GPU Preference is a Windows per-user, per-executable setting.
- It usually affects the next launch of the target application.
- It does not guarantee live switching for an already running process.
- It does not interact with NVIDIA Control Panel or vendor-specific global settings.
- It does not restart, kill, or relaunch the target app.

## Safety Guards

- Blocks PID 0 and PID 4.
- Blocks protected/system processes.
- Requires a non-empty `.exe` executable path.
- Blocks unavailable paths and stale process identity/path mismatches.
- Writes only under HKCU for the current user.
- Does not require administrator rights when HKCU write access is available.

## Still Not Implemented

- Profiles and rules.
- Autostart behavior.
- Settings persistence outside Windows GPU Preference.
- Registry changes outside per-user Windows Graphics Settings.
- Forced administrator elevation.
- Live GPU switching for running processes.
- NVIDIA Control Panel integration.
- Global GPU settings.

## Build Verification

- `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
- `cmake --build build --config Debug`
- `cmake --build build --config Release`

Generated executables:

- `C:\Vibe\WinProcessManager\build\Debug\WindowsProcessControlCenter.exe`
- `C:\Vibe\WinProcessManager\build\Release\WindowsProcessControlCenter.exe`

## Suggested Next Stage

Add an action history panel and richer process metadata so CPU Priority, End Process, Freeze/Resume, and GPU Preference outcomes are easier to audit from the UI.

## Task 10 - Final Polish, Regression Testing, Cleanup, and Release Readiness

Status: completed.

Prepared the project as a release-candidate baseline. The active architecture is now cleanly Win32 + WebView2 + native C++ process modules, with old Dear ImGui/DX11 reference files removed from the repository.

## Added Files

- `docs/RELEASE_CHECKLIST.md`

## Changed Files

- `CMakeLists.txt`
- `README.md`
- `PROGRESS.md`
- `src/platform/Win32Window.cpp`
- `web/index.html`
- `web/styles.css`
- `web/app.js`

## Removed Files

- `src/platform/D3D11Renderer.cpp`
- `src/platform/D3D11Renderer.h`
- `src/ui/UiLayer.cpp`
- `src/ui/UiLayer.h`
- `src/ui/UiTheme.cpp`
- `src/ui/UiTheme.h`

## What Was Polished

- Added best-effort DWM dark title bar support with safe fallback on unsupported Windows builds.
- Removed stale ImGui/DX11 CMake references and legacy source files.
- Tightened WebView2 UI copy for the release-candidate state.
- Improved keyboard focus visibility, scrollbars, sidebar text truncation, badge polish, small-screen layout behavior, and details-panel safety messaging.
- Removed a dead empty future-actions block from the frontend.

## Regression Test Scope

- App startup and WebView2 frontend loading.
- Process snapshot loading, Refresh, search, selection, and details panel.
- CPU Priority Normal -> High -> Normal and Realtime-without-checkbox blocking.
- End Process on a test `notepad.exe` process.
- Critical-process blocking for `explorer.exe`.
- Freeze and Resume on a test `cmd.exe` process.
- `Frozen by app` runtime badge.
- GPU Preference set/reset for a user process with a valid executable path.
- GPU Preference disabled for processes without a valid path.
- Best-effort resume of processes frozen by WPCC on shutdown.
- Debug and Release builds.

## Known Limitations

- GPU Preference may require restarting the target app and does not live-switch a running process.
- Freeze/Resume state is only tracked in the current WPCC session.
- Process tree operations, child-process termination, profiles, presets, and autostart are not implemented.
- Dark title bar is best-effort and depends on OS support for DWM immersive dark mode.

## Suggested Next Stage

Add a lightweight action history/audit panel and table sorting so process actions are easier to review without increasing system-level risk.

## Task 11 - Portable Release Packaging, Versioning, and Release Notes

Status: completed.

Prepared version `0.1.0` for portable distribution. The project now has a repeatable PowerShell packaging script that creates a clean portable folder and ZIP without committing build outputs.

## Added Files

- `RELEASE_NOTES.md`
- `scripts/package_release.ps1`

## Changed Files

- `.gitignore`
- `CMakeLists.txt`
- `README.md`
- `PROGRESS.md`
- `web/index.html`
- `web/styles.css`

## What Works

- Project version is `0.1.0` in CMake and visible in the WebView2 top bar.
- Release notes describe the `0.1.0` feature set, safety model, WebView2 Runtime requirement, and known limitations.
- Packaging script builds Release by default.
- Packaging script creates `dist/WindowsProcessControlCenter-0.1.0-portable/`.
- Packaging script copies `WindowsProcessControlCenter.exe`, `web/`, `README.md`, and `RELEASE_NOTES.md`.
- Packaging script creates `dist/WindowsProcessControlCenter-0.1.0-portable.zip`.
- `.gitignore` excludes `dist/` and generated ZIP files.
- MSVC runtime is configured for static linking to reduce external DLL needs for the portable package.

## Known Limitations

- Microsoft Edge WebView2 Runtime is still required on the target machine.
- No installer is generated; this is a portable ZIP package.
- No auto-update, signing, release tagging, or GitHub Release upload automation yet.

## Suggested Next Stage

Add release signing/version resource metadata and optionally automate GitHub Release creation after manual validation.
