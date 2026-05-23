# Windows Process Control Center 0.1.0

Release date: 2026-05-23

## Highlights

- Native Win32 desktop application with a WebView2 UI.
- Local vanilla HTML/CSS/JavaScript frontend copied beside the executable.
- Dashboard tab with responsive snapshot statistics, safety status, quick actions, last action status, and available controls overview.
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

## Known Limitations

- GPU Preference is per executable path and may require restarting the target app.
- GPU Preference does not guarantee live switching for an already running process.
- No profiles, rules, presets, autostart, or settings persistence yet.
- No process tree termination, child-process force kill, freeze tree, or resume tree.
- No NVIDIA Control Panel integration or global GPU setting changes.
