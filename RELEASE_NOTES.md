# WPCC 0.1.15-rc.1: Autoruns and update fixes

This is a pre-release. Automated Windows builds and tests must pass before publication. Real desktop use on Windows 10 and Windows 11 has not been verified. Browser UI tests use a simulated native host and simulated GitHub responses.

## Fixed

- Fixed Image Hijacks registry enumeration. The reader now requests the query access required by Windows and scans both registry views.
- Missing or empty registry locations are treated as normal. Read failures include the affected view, location, operation and Windows error code. Entries from readable locations remain visible.
- Fixed manual update checks. The default channel checks stable releases; the optional test channel includes pre-releases. Version comparison now understands release candidates and final releases.
- Manual checks show a newer release even if it was previously ignored. Network, timeout, malformed response and GitHub API errors are reported in the interface.
- Portable ZIP downloads open in your browser. Setup downloads use the installer flow, and installation requires an explicit click on Install now. Old automatic-install preferences no longer trigger unattended downloads.

## Autoruns improvements

- Compact rows show program, publisher, state, startup source and image path. Details expand to show full paths, commands, registry locations and change restrictions.
- Added plain descriptions of startup categories and what disabling an entry means.
- Added pagination with 100 entries per page while keeping search, category filters and sorting.
- Everything includes read-only entries. Protected entries remain read-only.
- Publisher names come from file metadata and are not signature verification. Unknown publishers are not automatically marked as unsafe.

## Downloads

- Setup: `WindowsProcessControlCenter-v0.1.15-Setup.exe`.
- Portable: `WindowsProcessControlCenter-v0.1.15-Portable.zip`. Extract all files, keeping the web folder beside the executable.
- Internal EXE version: `0.1.15.0`. Release identity used by the update checker: `0.1.15-rc.1`.
- To see test updates in this version, select Stable and test releases in Settings. Earlier versions that only check stable releases may need a manual download of this pre-release.

## Test scope and limitations

The Windows test suite checks a real registry fixture, reproduces the old missing-access error, scans real IFEO with both view flags, and simulates a denied view and a partial value read failure. Real IFEO keys are never modified by these tests. IFEO is a shared location on modern Windows, so the view flags can read the same underlying data.

Update tests cover stable releases, release candidates, no update, empty results, ignored versions, HTTP errors, invalid JSON and network failures. Browser tests click the actual manual-check button and check modal visibility, notes, Setup confirmation and Portable links. They also render 1,200 entries, long paths, read-only entries and a smaller window.

The installer and Portable payload are checked against the same Release executable. Silent installation and uninstall run on a clean CI machine. Real user-specific ACLs, production downloads, the native WebView2 dialog, autostart after logon/reboot and uninstall across multiple user profiles still require desktop testing.

---

# WPCC 0.1.14-rc.1: Startup fix (pre-release)

This is a test release. Automated builds and tests passed, but **startup after signing out and restarting has not yet been verified** with real user logon on Windows 10 or Windows 11.

## Changes

- Replaced the Run registry entry with a Task Scheduler logon task so WPCC can start with the required administrator privileges for the correct user.
- The startup switch reflects the state read from Windows. Configuration errors and settings save failures appear in the interface, and rapid changes are saved in order.
- Matching, enabled legacy entries are migrated. Reading settings does not recreate a missing or disabled task.
- A tray icon failure leaves the application window accessible. The icon is restored after Explorer restarts.
- Added startup.log diagnostics, a task verification script, and cleanup of WPCC startup entries during uninstall.
- Fixed packaging paths and tool error handling. GitHub Actions builds Debug and Release, runs tests, and creates Setup and Portable packages from the same Release executable.

## Downloads and limitations

- Setup: `WindowsProcessControlCenter-v0.1.14-Setup.exe`.
- Portable: `WindowsProcessControlCenter-v0.1.14-Portable.zip`. Extract the entire archive and keep the `web` folder beside the executable.
- Internal executable and installer version: `0.1.14.0`. GitHub release tag: `v0.1.14-rc.1`.
- Startup requires signing in with your own administrator account. Entering another administrator's credentials in UAC does not configure startup for the original user.
- After moving the Portable folder, turn startup off and back on. The packages are unsigned and require WebView2 Runtime.
- Logic tests do not replace real logon, migration, tray failure, or uninstall tests across multiple user profiles. The full test matrix is in `docs/AUTOSTART_FIX.md`.

## Quick test on your computer

1. Run the new version and enable "Start with Windows". Wait for confirmation without a warning. Optionally enable minimizing to the tray.
2. Sign out and sign back in. After about 10 to 30 seconds, check that exactly one WPCC process is running, its icon or window is accessible, and your settings are preserved.
3. Separately restart the computer and repeat the checks after signing in.
4. Disable startup, close WPCC, and sign in again. The application should not start.
5. If anything fails, keep `%LOCALAPPDATA%\WindowsProcessControlCenter\startup.log` and the warning shown in Settings. The Portable package includes `scripts/verify_startup.ps1` to check the task configuration. It does not replace a real restart test.

## Automated validation

Debug and Release builds, CTest, package verification, and a silent install/uninstall test passed: [GitHub Actions](https://github.com/Zayoooh1/WindowsProcessControlCenter/actions/runs/37308509816).

Release binaries were built from commit `90e56512ca045c08fba3d8149c6b801f3ac88486`.

---

# Windows Process Control Center 0.1.2

Release date: 2026-05-25

**Update: Native Downloader Engine, Process Picker & Table Enhancements.**

- **Native Update Downloader Engine**: Overhauled the update delivery pipeline to execute downloads directly on the C++ backend using native `URLDownloadToFileW`. This completely suppresses the default Edge WebView2 download dialogs and overrides Microsoft Defender SmartScreen blocks for unsigned executables.
- **Real-Time Progress Tracking & Thread Safety**: Implemented the `IBindStatusCallback` interface to intercept and report download bytes dynamically. Progress is dispatched to the main UI thread via thread-safe window messaging (`WM_DOWNLOAD_PROGRESS` & `WM_DOWNLOAD_COMPLETE`) and memory-guarded pointers, preventing COM STA threading violations or application hangs.
- **Interactive Update Modal & Auto-Install**: Redesigned the "Update available" modal to transition into a smooth progress track during download. On completion, it triggers an instant installation sequence using `ShellExecuteW` to launch the installer and exits the application cleanly.
- **Unignored Version Revocation**: Added a reset button in Settings → Updates to clear the ignored release version, resolving the lock trap and permitting users to reinstall or review ignored versions.
- **Profile Process Picker**: Integrated a "Select running process" dropdown in the Profile Creation panel to pre-populate executable paths and process names from active processes.
- **Hardened Process Exclusions**: Secured the process picker by excluding low PID system processes, protected tasks, and system folder paths (e.g., `System32`, `SysWOW64`, `SystemApps`) using path normalization and fallback checking for empty or unavailable paths.
- **Table Sorting & Column Toggles**: Implemented column-header sorting on the process table by CPU Priority class and GPU Preference, and introduced a toggle to show/hide the Process ID (PID) column.
- **UI Layout & Popover Stabilization**: Replaced the sidebar-based safety note with an interactive popover tooltip positioned safely to avoid container clipping. Restructured the profiles list to prevent name truncation and badge overlaps.

## Portable Package (0.1.2)

The portable package is named:

```text
WindowsProcessControlCenter-0.1.2-portable.zip
```

Unzip it and run:

```text
WindowsProcessControlCenter.exe
```

The `web/` folder must remain beside the executable.

## Installer Package (0.1.2)

An optional Windows installer is also available:

```text
WindowsProcessControlCenter-0.1.2-setup.exe
```

The installer installs per user by default, supports custom install locations, and configures shortcuts.

---

# Windows Process Control Center 0.1.1

Release date: 2026-05-24

**Update: Profiles Apply v1 and Bug Fixes.**

- **Profiles Apply v1**: Added manual profile application. Clicking "Apply profile" now parses profiles on the native C++ backend, searches running processes by executable path or process name, normalizes paths/names case-insensitively, and sets CPU priority class via WinAPI.
- **Diagnostics Logging**: Added comprehensive diagnostic logging for profile application actions and Win32 success/error messages.
- **Fixed Browse Button**: Fixed the Browse button in profile creation. It now opens the native Windows Open File Dialog correctly to select executable paths.
- **Fixed Startup Loading**: Fixed startup process loading. The process list now loads automatically after launching the app.
- **Fixed Default Sorting**: Fixed default process ordering. Processes are now sorted alphabetically, case-insensitive.
- **Kept Refresh**: Kept manual Refresh behavior intact.
- **Improved Startup**: Improved WebView2 bridge startup handling.

## Portable Package (0.1.1)

The portable package is named:

```text
WindowsProcessControlCenter-0.1.1-portable.zip
```

Unzip it and run:

```text
WindowsProcessControlCenter.exe
```

The `web/` folder must remain beside the executable.

## Installer Package (0.1.1)

An optional Windows installer is also available:

```text
WindowsProcessControlCenter-0.1.1-setup.exe
```

The installer installs per user by default, supports custom install locations, and configures shortcuts.

---

# Windows Process Control Center 0.1.0

Release date: 2026-05-23

**Update: Profiles v1 localStorage foundation.**

- Implemented the persistent Profiles v1 management UI in the Rules / Profiles tab.
- Profiles can be created, updated, and deleted, persisting in WebView2 `localStorage` under a single `wpcc.profiles` registry key.
- Profiles target apps using full executable path or process name rather than volatile Process IDs (PIDs).
- Supports mapping CPU Priority presets and Graphics Preference classes per application.
- Realtime priority saves are guarded by an interactive performance risk confirmation check.
- Interactive custom modal overlays manage edit, cancel, and double-check delete confirmation routines cleanly without native browser popups.
- Fallback mechanisms handle corrupted storage values gracefully and switch seamlessly to session-local in-memory tracking if localStorage is unavailable.

**Update: GitHub Releases update checker implemented.**

- The frontend now performs real GitHub Releases checks against `Zayoooh1/WindowsProcessControlCenter` and shows update status in Settings → Updates.
- Local update state is stored in `localStorage` as `wpcc.updateState` with fields: `lastCheckedAt`, `lastKnownVersion`, `latestReleaseUrl`, `ignoredVersion`.
- A HTTP 404 response from the GitHub Releases API is surfaced with a friendly message explaining that the repo may be private or no public release exists. The frontend checker requires a public Releases endpoint.
 - Added an in-app update prompt UX: when a newer release is detected the app shows a dialog with current/latest version, release title, short notes, release URL, and detected assets. Buttons let the user open the release page, download assets (open in browser), remind later, ignore this version, or disable update checks. Ignored versions persist in settings and update state.
- Automatic checks respect the existing `wpcc.settings.updateChecksEnabled` and `updateCheckInterval` settings. Manual "Check for updates now" performs an immediate check.
- Version parsing strips leading `v` and compares `major.minor.patch` numerically. Prerelease releases are ignored. No automatic download or installation is performed.

**Earlier update: Windows 10 compatibility and DPI/responsive audit.**

- DPI awareness now uses a safe fallback chain: per-monitor v2 -> per-monitor v1 -> system DPI aware, ensuring compatibility across Windows 10 1607+.
- DWM dark title bar already had a safe fallback (attribute 20 -> 19) for older Windows 10 builds. Verified unchanged.
- Tray icon implementation verified: uses standard `Shell_NotifyIconW`, no Windows 11-only dependencies.
- Inno Setup installer verified: per-user install, no admin requirement, `x64compatible` architecture.
- Responsive CSS improved with additional short-height breakpoints for 680px and 600px viewport heights.
- All tabs (Dashboard, Processes, Rules/Profiles, Settings, About) reviewed for responsive behavior at 1280x720, 1366x768, 1600x900, and 1920x1080 at 100%, 125%, and 150% DPI scaling.
- Tables and details panels use overflow scroll and remain usable at all tested resolutions.
- README now includes a dedicated Compatibility section with Windows 10 fallback notes, DPI recommendations, and WebView2 Runtime requirement documentation.
- **Note:** Windows 10 was not tested on a real device or VM in this task. Compatibility changes are based on code review, documented API fallback behavior, and Windows 11 validation. Real Windows 10 testing is still recommended.

**Earlier update: Added Windows resource metadata and application icon.**

## Highlights

- Native Win32 desktop application with a WebView2 UI.
- Local vanilla HTML/CSS/JavaScript frontend copied beside the executable.
- Dashboard tab with responsive snapshot statistics, safety status, quick actions, last action status, and available controls overview.
- Settings tab with frontend-only UI preferences saved in WebView2 `localStorage`.
- About tab displaying application info, versioning, tech stack details, and known limitations.
- Rules / Profiles tab design prototype describing planned features (Auto-apply priority/GPU, presets, safe startup, import/export, conflict safeguards, and a disabled Create profile control).
- Real Windows process listing through C++/WinAPI.
- Search by PID, process name, or executable path.
- Details panel with path, CPU priority, runtime state, access status, admin hint, and GPU Preference.
- CPU Priority control for accessible user processes.
- Realtime priority requires explicit risk confirmation in the UI and backend.
- Safe End Process action with a confirmation modal.
- Safe Freeze and Resume actions using documented thread APIs.
- Resume only restores threads frozen by this application during the current session.
- Best-effort automatic resume of processes frozen by this app when WPCC closes.
- Windows GPU Preference management per executable path through current-user Windows Graphics Settings.
- Critical, protected, inaccessible, and self processes are blocked from destructive actions.
- Destructive action confirmations remain required and cannot be disabled from Settings.

## Portable Package

The portable package is named:

```text
WindowsProcessControlCenter-0.1.0-portable.zip
```

Unzip it and run:

```text
WindowsProcessControlCenter.exe
```

The `web/` folder must remain beside the executable.

## Installer Package

An optional Windows installer is also available:

```text
WindowsProcessControlCenter-0.1.0-setup.exe
```

The installer uses Inno Setup, installs per user by default, supports custom install locations, can create desktop and Start Menu shortcuts, can optionally start the app with Windows through HKCU Run, and supports normal Windows uninstall.

## Requirements

- Windows 10 or newer.
- Microsoft Edge WebView2 Runtime.
- The WebView2 Runtime is usually already present on Windows 11 and many Windows 10 installations.
- If the UI does not load, install the Microsoft Edge WebView2 Evergreen Runtime from Microsoft.

## Safety Notes

- End Process is destructive and requires typing the selected process name or PID.
- Freeze can make the target application stop responding until Resume is used.
- Realtime CPU priority can make Windows less responsive and requires explicit confirmation.
- Critical Windows processes such as `explorer.exe`, `svchost.exe`, `lsass.exe`, and `csrss.exe` are blocked.

## Windows Resource Metadata and Icon Integration

- Application icon with multi-resolution support: 16×16, 24×24, 32×32, 48×48, 64×64, 128×128, and 256×256.
- Windows version resource embedded in the executable:
  - File version 0.1.0, product version 0.1.0.
  - Product name, file description, original filename, and copyright metadata.
- Win32 window class uses the icon for the title bar, taskbar, and Alt+Tab.
- File Explorer shows the icon on the executable and shortcuts.
- Inno Setup installer uses the icon for the installer executable and setup wizard.
- Version information visible in File Properties > Details and Apps & Features.
- System tray icon with right-click context menu (Open, Refresh process snapshot, Exit).
- Tray icon uses the app icon resource and shows tooltip "Windows Process Control Center".
- Double-click or single-click on tray icon restores/focuses the main window.
- Tray icon is removed cleanly when the application exits.

## Known Limitations

- GPU Preference is per executable path and may require restarting the target app.
- GPU Preference does not guarantee live switching for an already running process.
- Native profiles, rules, presets, native settings file, or backend settings persistence yet (Rules / Profiles is currently a frontend-only design prototype).
- No process tree termination, child-process force kill, freeze tree, or resume tree.
- No NVIDIA Control Panel integration or global GPU setting changes.
