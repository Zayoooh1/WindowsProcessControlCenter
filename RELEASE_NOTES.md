# Windows Process Control Center 0.1.0

Release date: 2026-05-23

**Update: Windows 10 compatibility and DPI/responsive audit.**

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
