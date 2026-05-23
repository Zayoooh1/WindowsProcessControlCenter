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

## Implemented in 0.1.0

- Real process listing with PID, name, executable path, CPU priority, access status, admin hint, runtime state, and GPU Preference.
- Refresh and frontend-side search by PID, process name, or executable path.
- CPU Priority control for accessible user processes.
- Realtime priority confirmation in the UI and backend.
- Safe End Process action with explicit confirmation.
- Safe Freeze and Resume actions for single user processes in the current app session.
- Best-effort resume of processes frozen by this app when WPCC closes.
- Windows GPU Preference management per executable path through current-user Windows Graphics Settings.
- Blocking for critical, protected, inaccessible, and self processes.
- Portable Release folder and ZIP packaging.

## Documentation

- `README.md` covers build, run, portable packaging, WebView2 Runtime requirements, and safety notes.
- `RELEASE_NOTES.md` summarizes version `0.1.0`.
- `docs/RELEASE_CHECKLIST.md` contains the manual release validation checklist.
- `docs/GUI_LIBRARY_DECISION.md` records why the active UI layer is WebView2.

## Known Limitations

- WebView2 Runtime must be installed on the target machine.
- GPU Preference may require restarting the target app and does not live-switch a running process.
- Freeze/Resume state is session-local.
- No profiles, rules, presets, autostart, installer, code signing, or auto-update yet.
- No process tree termination, child-process force kill, freeze tree, or resume tree.
- No NVIDIA Control Panel integration or global GPU setting changes.

## Suggested Next Steps

- Add Windows version resource metadata and optional code signing.
- Add a lightweight action history/audit panel.
- Add table sorting and richer read-only metadata.
- Automate GitHub Release creation after manual validation.
