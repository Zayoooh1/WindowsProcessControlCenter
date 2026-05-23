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

## Suggested Next Steps

- Add a lightweight action history/audit panel.
- Add table sorting and richer read-only metadata.
- Add native settings/config persistence if UI preferences should move beyond WebView2 localStorage.
- Automate GitHub Release creation after manual validation.
