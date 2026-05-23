# Project Status

Windows Process Control Center is currently at version `0.1.0`.

This file is intentionally kept short for public-facing repository history. Detailed internal task prompts and implementation logs are not part of the maintained documentation.

## Current Architecture

- Native C++20 Win32 desktop application.
- WebView2 frontend loaded from local `web/` files.
- Vanilla HTML/CSS/JavaScript UI with no external CDN assets.
- Native C++/WinAPI backend for process enumeration and guarded process actions.
- CMake build with automatic WebView2 SDK download.
- Portable release packaging through `scripts/package_release.ps1`.
- Optional Inno Setup installer packaging through `scripts/build_installer.ps1`.

## Implemented in 0.1.0

- Real process listing with PID, name, executable path, CPU priority, access status, admin hint, runtime state, and GPU Preference.
- Dashboard tab with responsive current snapshot statistics, safety status, quick actions, last action summary, and available controls overview.
- Settings tab with frontend-only UI preferences saved in `localStorage` under `wpcc.settings`.
- About tab displaying version, tech stack description, safety notes, and known limitations.
- Rules / Profiles tab design prototype describing planned features (Auto-apply priority/GPU, presets, safe startup, import/export, conflict safeguards, and a disabled Create profile button).
- Settings include start screen, compact process table, executable path column visibility, details-panel safety notes, reduced visual effects, and locked destructive-action confirmations.
- Refresh and frontend-side search by PID, process name, or executable path.
- CPU Priority control for accessible user processes.
- Realtime priority confirmation in the UI and backend.
- Safe End Process action with explicit confirmation.
- Safe Freeze and Resume actions for single user processes in the current app session.
- Best-effort resume of processes frozen by this app when WPCC closes.
- Windows GPU Preference management per executable path through current-user Windows Graphics Settings.
- Blocking for critical, protected, inaccessible, and self processes.
- Portable Release folder and ZIP packaging.
- Optional per-user Windows installer with shortcuts, Start Menu entry, optional HKCU autostart, and uninstall support.

## Documentation

- `README.md` covers build, run, portable packaging, WebView2 Runtime requirements, and safety notes.
- `RELEASE_NOTES.md` summarizes version `0.1.0`.
- `docs/RELEASE_CHECKLIST.md` contains the manual release validation checklist.
- `docs/GUI_LIBRARY_DECISION.md` records why the active UI layer is WebView2.

## Known Limitations

- WebView2 Runtime must be installed on the target machine.
- GPU Preference may require restarting the target app and does not live-switch a running process.
- Freeze/Resume state is session-local.
- Settings are local WebView2 UI preferences only; there is no native config file yet.
- Native profiles, rules, presets, code signing, or auto-update yet (Rules / Profiles is currently a frontend-only design prototype).
- No process tree termination, child-process force kill, freeze tree, or resume tree.
- No NVIDIA Control Panel integration or global GPU setting changes.
- The installer build requires Inno Setup 6 locally; WebView2 Runtime is still required on the installed machine.

## Implemented Since 0.1.0

- Application icon (`assets/icon.ico`) with multiple resolutions: 16×16, 24×24, 32×32, 48×48, 64×64, 128×128, 256×256.
- Windows resource metadata (`resources/WindowsProcessControlCenter.rc`, `resources/resource.h`):
  - File version 0.1.0 and product version 0.1.0.
  - Product name, file description, original filename, and copyright.
- Win32 window class icon (large and small) loaded from resources.
- Inno Setup installer uses the icon for the installer executable and installed shortcuts.
- Version resource integration for Apps & Features and File Properties details.
- System tray icon (`src/platform/TrayIcon.h`, `src/platform/TrayIcon.cpp`):
  - Tray icon visible while the app is running.
  - Right-click context menu: Open, Refresh process snapshot, Exit.
  - Double-click or single-click restores/focuses the main window.
  - Clean removal on app exit.

## Update Checker

- Implemented real GitHub Releases scanning in the WebView2 frontend. The app queries the public API `https://api.github.com/repos/Zayoooh1/WindowsProcessControlCenter/releases/latest` for release metadata and shows update status in Settings → Updates.
- The local update state is stored under `wpcc.updateState` in `localStorage` and includes `lastCheckedAt`, `lastKnownVersion`, `latestReleaseUrl`, and `ignoredVersion`.
- Automatic checking respects `wpcc.settings.updateChecksEnabled` and `updateCheckInterval` (3d/weekly/monthly). Manual "Check for updates now" is enabled and always performs a check.
- The checker performs semantic version parsing (strips leading `v`, compares major/minor/patch numerically) and ignores prerelease releases. Friendly error messages are shown for network/API/JSON errors and timeouts.
  - Note: a HTTP 404 from the GitHub API is reported as a friendly message explaining that the repository may be private or that no public release exists. The frontend checker requires a public Releases endpoint.
- Implemented an in-app update prompt dialog when a newer release is detected (manual or auto check). The dialog shows version details, release notes summary, asset links, and buttons: Open release, Download installer (opens browser), Download portable ZIP (opens browser), Remind me later, Ignore this version, Disable update checks.
- Ignored version is stored in both `wpcc.settings.ignoredUpdateVersion` and `wpcc.updateState.ignoredVersion` and is respected by auto and manual checks.

## Windows 10 Compatibility and DPI/Responsive Audit

- DPI awareness uses a safe fallback chain: per-monitor v2 -> per-monitor v1 -> system DPI aware (`SetProcessDPIAware`).
- Dark title bar already had a safe fallback (attribute 20 -> 19) for older Windows 10 builds. No change needed.
- Tray icon verified compatible: standard `Shell_NotifyIconW` APIs, no Windows 11-only code paths.
- Installer verified: per-user install (`PrivilegesRequired=lowest`), `x64compatible` architecture, no admin requirement.
- Responsive CSS reviewed and improved with additional short-height breakpoints for 680px and 600px viewport heights.
- README now includes a dedicated Compatibility section listing supported OS, DPI recommendations, WebView2 Runtime requirement, and Windows 10 fallback notes.
- **Known limitations:**
  - Real Windows 10 VM/device testing was not performed. Compatibility is based on code review and documented API behavior only.
  - At very high DPI scaling (>200%) combined with 1280x720, the sidebar is hidden (760px CSS breakpoint) and there is no alternative navigation. This is acceptable for current 0.1.0 scope.

## Profiles v1 Local Storage Foundation

- Fully replaced the planned Rules / Profiles view placeholder with a functional, premium persistent management UI.
- Implemented persistent profile configurations under WebView2 `localStorage` using the single `wpcc.profiles` registry key.
- Supports the following core fields per profile item:
  - `id`: unique UUID/timestamp string identifier.
  - `name`: user display name.
  - `targetExePath`: full executable file path.
  - `targetProcessName`: executable filename fallback.
  - `matchMode`: target mapping style (`path` or `name`).
  - `cpuPriority`: target priority preset class (`DoNotChange`, `High`, `AboveNormal`, `Normal`, `BelowNormal`, `Idle`, or `Realtime`).
  - `gpuPreference`: target Graphics Preference class (`DoNotChange`, `SystemDefault`, `PowerSaving`, or `HighPerformance`).
  - `applyToFamily`: whether setting applies across family executable instances (boolean).
  - `autoApply`: whether preset dynamically auto-applies on recognition (boolean, visible but inactive).
  - `allowRealtime`: safeguard check indicating permission to map Realtime class (boolean).
  - `notes`: freeform notes textarea string.
  - `createdAt` and `updatedAt` ISO date records.
- Implemented safety validation and parser safeguards for missing JSON configurations, old file schemas, unexpected field schemas, or corrupted storage profiles.
- Implemented in-memory fallback routines that support full profile addition, editing, and deletion during a session if localStorage is completely disabled or blocked by WebView2.
- Interactive safety check block prevents saving a profile with a `Realtime` class unless standard risks are explicitly acknowledged.
- Beautiful confirmation modal overlays prevent deletion of profile records without dynamic, double-check actions (no browser native alerts).

### Known Limitations

- All profiles matchings and setups are informational. No native process prioritization or GPU preference auto-applications are performed yet.
- C++ native background monitoring and file-based `profiles.json` native saves are planned for subsequent milestones.

## Suggested Next Steps

- Add C++ native profiles.json backings and auto-apply rules engine.
- Add lightweight action history/audit panel.
- Add table sorting and richer read-only metadata.
- Perform real Windows 10 VM/device testing to validate the compatibility changes.
