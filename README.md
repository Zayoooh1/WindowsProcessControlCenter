# Windows Process Control Center

Version: `0.1.0`

Windows Process Control Center is a C++20 Win32 desktop application for Windows process inspection and controlled process-management workflows. The current UI is rendered with Microsoft Edge WebView2 using local vanilla HTML, CSS, and JavaScript, while the backend remains native C++/WinAPI.

The app can display real process snapshots, summarize the current snapshot on a Dashboard tab, store frontend-only UI preferences in a Settings tab, change the CPU priority class of accessible user processes, safely terminate selected non-critical user processes after explicit confirmation, freeze/resume selected user processes during the current app session, and set Windows GPU Preference per executable path. It blocks known critical Windows processes and does not persist native config files, custom profiles, or app presets yet.

## Compatibility

- **Supported OS:** Windows 10 x64 (20H1 / build 19041 or newer recommended) and Windows 11 x64.
- **Windows 10 fallback notes:**
  - DPI awareness uses a safe fallback chain: per-monitor v2 -> per-monitor v1 -> system DPI aware. Compatible with Windows 10 1607+.
  - Dark title bar uses the `DWMWA_USE_IMMERSIVE_DARK_MODE` attribute (20) where available, with a fallback to attribute 19 for older Windows 10 builds. The title bar always renders correctly regardless of dark-mode support.
  - Tray icon uses standard `Shell_NotifyIconW` APIs with no Windows 11-only behavior.
  - All feature-level code paths (CPU priority, freeze/resume, GPU preference, process termination) are Win32 API-based and do not depend on Windows 11.
- **WebView2 Runtime requirement:** The app requires Microsoft Edge WebView2 Evergreen Runtime. It ships with Windows 11 and most Windows 10 installations. The installer shows a notice if it is not detected.
- **DPI / resolution recommendations:**
  - The app performs best at 1280x720 or higher with 100%–150% DPI scaling.
  - The minimum window size is 1060 x 680 physical pixels.
  - All tabs are scrollable and tested at 1280x720, 1366x768, 1600x900, and 1920x1080.
- **Windows 10 real device testing:** Compatibility changes in this release were based on code review, documented API fallback behavior, and Windows 11 validation. Real Windows 10 VM or device testing is still recommended but was not performed in this task.

## Requirements

- Windows 10 x64 (20H1 build 19041 or newer recommended) or Windows 11 x64
- Visual Studio 2022 with MSVC C++ tools
- Windows SDK
- CMake 3.24 or newer
- Internet access during first CMake configure, so CMake can download the WebView2 SDK NuGet package
- Microsoft Edge WebView2 Runtime installed on the machine

## WebView2 Runtime

The app uses the Evergreen WebView2 Runtime installed on Windows. Many Windows 10/11 systems already have it.

Check locally:

```powershell
Get-ChildItem "C:\Program Files (x86)\Microsoft\EdgeWebView\Application" -ErrorAction SilentlyContinue
```

If it is missing, install the Evergreen WebView2 Runtime from Microsoft:

- [Microsoft Edge WebView2 download page](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)
- [Microsoft WebView2 deployment documentation](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/deployment-distribution)

## WebView2 SDK

CMake downloads the Microsoft.Web.WebView2 SDK package automatically from NuGet:

- Package: `Microsoft.Web.WebView2`
- Version: `1.0.3967.48`
- NuGet page: [Microsoft.Web.WebView2](https://www.nuget.org/packages/Microsoft.Web.WebView2/)

The package is extracted under `build/_deps/` and linked through the static WebView2 loader library.

## Build

From the repository root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
cmake --build build --config Release
```

The build copies the local frontend files from `web/` into the output folder beside the executable:

```text
build/Debug/web/index.html
build/Debug/web/styles.css
build/Debug/web/app.js
```

The same copy step runs for Release.

## Portable Release Package

Create a portable Release package from the repository root:

```powershell
.\scripts\package_release.ps1
```

The script configures/builds Release by default, creates a clean portable folder, copies the executable, copies the local `web/` frontend, copies release documentation, and creates a ZIP:

```text
dist/WindowsProcessControlCenter-0.1.0-portable/
dist/WindowsProcessControlCenter-0.1.0-portable.zip
```

The portable folder contains:

- `WindowsProcessControlCenter.exe`
- `web/index.html`
- `web/styles.css`
- `web/app.js`
- `README.md`
- `RELEASE_NOTES.md`

The `web/` folder must stay beside the executable. The app does not load external frontend assets, CDNs, or web scripts.

## Installer Build

Windows Process Control Center can also be packaged as a Windows installer using Inno Setup 6.

Install Inno Setup from:

- [Inno Setup](https://jrsoftware.org/isinfo.php)

The build script looks for `ISCC.exe` in `PATH` and in standard install locations:

- `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`
- `C:\Program Files\Inno Setup 6\ISCC.exe`

Build the installer from the repository root:

```powershell
.\scripts\build_installer.ps1
```

The generated installer is written to:

```text
dist/installer/WindowsProcessControlCenter-0.1.0-setup.exe
```

The installer is per-user by default and does not require administrator rights for the default location:

```text
%LOCALAPPDATA%\Programs\WindowsProcessControlCenter
```

It lets the user choose the install location, create a desktop shortcut, create a Start Menu folder, and optionally start the app with Windows through the current user's `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` key. Uninstall removes installed files and the optional autostart entry.

## Distribution Options

- Portable ZIP: no installation. Unzip the package, keep `web/` beside the executable, and run `WindowsProcessControlCenter.exe`.
- Installer EXE: normal per-user installation with optional shortcuts, optional autostart, and Windows uninstall support.

## Run

After a Debug build:

```powershell
.\build\Debug\WindowsProcessControlCenter.exe
```

After a Release build:

```powershell
.\build\Release\WindowsProcessControlCenter.exe
```

After creating the portable package:

```powershell
.\dist\WindowsProcessControlCenter-0.1.0-portable\WindowsProcessControlCenter.exe
```

## Current Scope

Implemented so far:

- Win32 desktop application entry point and native window.
- WebView2 host for local frontend rendering.
- Vanilla HTML/CSS/JS frontend in `web/`.
- Best-effort dark native title bar on supported Windows builds, with a normal Windows title bar fallback.
- C++ to JavaScript message bridge for process snapshots.
- JavaScript to C++ message bridge for `refreshProcesses`.
- JavaScript to C++ message bridge for `setCpuPriority`.
- JavaScript to C++ message bridge for `terminateProcess`.
- Dashboard tab with responsive snapshot statistics, safety status, quick actions, last action status, and available controls overview.
- Settings tab with frontend-only preferences stored in `localStorage` under `wpcc.settings`.
- About tab with application details, version information, safety model description, tech stack details, and known limitations.
- Rules / Profiles tab interface prototype outlining planned automation rules (Auto-apply priority/GPU, presets, safe startup, import/export, conflict safeguards, and a disabled Create profile control).
- Settings for start screen, compact process table rows, executable path column visibility, details-panel safety notes, and reduced visual effects.
- Read-only process enumeration using Windows APIs.
- CPU priority changes for accessible processes through `OpenProcess` and `SetPriorityClass`.
- Realtime priority safeguard requiring explicit frontend confirmation and backend validation.
- Safe single-process termination for accessible non-critical user processes through `OpenProcess` and `TerminateProcess`.
- End Process confirmation modal requiring the exact process name or PID when no name is available.
- Safe Freeze action using documented thread APIs: `CreateToolhelp32Snapshot`, `Thread32First`, `Thread32Next`, `OpenThread`, and `SuspendThread`.
- Safe Resume action that resumes only threads frozen by this app in the current session.
- Runtime status badges: `Running` and `Frozen by app`.
- Automatic best-effort resume of processes frozen by this app when Windows Process Control Center closes.
- Windows GPU Preference management per executable path through `HKCU\Software\Microsoft\DirectX\UserGpuPreferences`.
- GPU preference values use the Windows Graphics Settings convention: no registry value means `SystemDefault`, `GpuPreference=1;` means `PowerSaving`, and `GpuPreference=2;` means `HighPerformance`.
- Backend guards that block protected, inaccessible, self, and critical Windows processes.
- UI warnings for destructive or risky actions:
  - Realtime priority requires an explicit risk checkbox and backend confirmation.
  - End Process requires typing the exact process name or PID in a custom confirmation modal.
  - Freeze requires typing the exact process name or PID and can temporarily stop the target app's UI.
  - Freeze state is session-local; WPCC tries to resume processes it froze when closing.
  - Destructive confirmations are always required and cannot be disabled in Settings.
- Process filtering by name, PID, or executable path.
- Process details panel with executable path, CPU priority, access status, admin requirement hint, and access error details when available.
- Windows resource metadata (file version, product version, description, copyright) embedded in the executable.
- Application icon (`assets/icon.ico`) with multi-resolution support: 16×16, 24×24, 32×32, 48×48, 64×64, 128×128, 256×256.
- Win32 window class icon (title bar, taskbar, Alt+Tab) loaded from resources.
- Installer uses the application icon for the setup executable and created shortcuts.
- System tray icon with right-click context menu (Open, Refresh process snapshot, Exit) and double-click to restore. Removed cleanly on exit.

## Settings

The Settings tab stores local interface preferences in WebView2 `localStorage` using a single key:

```text
wpcc.settings
```

These settings are frontend-only. They do not create a native configuration file and do not change backend process-management safeguards.

Available UI preferences:

- Start screen: choose Dashboard or Processes on startup.
- Compact process table: use tighter process table rows.
- Show executable path column: hide or show the path column while keeping the path visible in the details panel.
- Show safety notes in details panel: show or hide the Safety model section.
- Reduce visual effects: reduce UI transitions and motion-sensitive effects.
- Always require confirmation for destructive actions: visible as enabled and locked. End Process and Freeze confirmations cannot be disabled.

If `localStorage` is missing, blocked, or contains unreadable settings JSON, the app falls back to safe defaults and the Settings tab shows a notice.

Not implemented yet:

- Process tree termination or force-killing child processes.
- Freeze/resume of process trees or child processes.
- Live GPU switching for an already running process.
- Native settings/config file persistence.
- Native profiles, rules, autostart, elevation, or admin workflows (Rules / Profiles is currently a frontend-only prototype).

GPU Preference may require restarting the target application and does not guarantee live switching for an already running process.

## Safety Notes

- Critical Windows processes such as `System`, `Registry`, `smss.exe`, `csrss.exe`, `wininit.exe`, `winlogon.exe`, `services.exe`, `lsass.exe`, `svchost.exe`, `fontdrvhost.exe`, `dwm.exe`, `explorer.exe`, and `audiodg.exe` are blocked from destructive actions.
- Realtime CPU priority can make the system less responsive. The UI and backend both require explicit confirmation before it is applied.
- End Process terminates only the selected process, not its child processes, and should be used only when unsaved work is not at risk.
- Freeze suspends current threads for one selected process and Resume only restores threads frozen by this app in the current session.
- GPU Preference writes only the current user's Windows Graphics Settings entry under `HKCU`.

## Project Structure

```text
src/
  app/        Application lifecycle and main loop.
  core/       Process model and read-only process provider.
  platform/   Win32 window wrapper.
  ui_web/     WebView2 host and message bridge.
web/          Local vanilla HTML/CSS/JS frontend loaded by WebView2.
docs/         Project documentation.
```
