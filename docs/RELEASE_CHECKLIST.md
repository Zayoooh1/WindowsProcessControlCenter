# Release Checklist

Use this checklist before tagging or backing up a release-candidate build.

## Build

- Configure with Visual Studio 2022 x64:
  `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
- Build Debug:
  `cmake --build build --config Debug`
- Build Release:
  `cmake --build build --config Release`
- Confirm `web/index.html`, `web/styles.css`, and `web/app.js` are copied beside each executable.

## Smoke Test

- App starts without a WebView2 initialization error.
- WebView2 loads the local frontend.
- The process list loads automatically.
- Refresh reloads the process snapshot.
- Search filters by PID, process name, and executable path.
- Selecting a process updates the details panel.

## Process Actions

- CPU Priority changes a safe user process from Normal to High and back to Normal.
- Realtime priority cannot be applied unless the risk checkbox is checked.
- End Process requires the confirmation modal and can end a test `notepad.exe`.
- End Process is disabled or blocked for critical processes such as `explorer.exe`.
- Freeze requires the confirmation modal and can freeze a test `cmd.exe` or `notepad.exe`.
- Resume is enabled only for processes frozen by this app.
- `Frozen by app` appears after Freeze and disappears after Resume.
- Closing WPCC tries to resume processes frozen by WPCC in the current session.
- GPU Preference can be set for a process with a valid `.exe` path.
- Reset to System default removes the HKCU GPU preference value.
- GPU Preference is disabled when the executable path is unavailable.

## Safety Boundaries

- Do not test destructive actions on `explorer.exe`, `svchost.exe`, `lsass.exe`, `csrss.exe`, antivirus processes, drivers, or protected/system processes.
- Use disposable user processes such as `notepad.exe`, `cmd.exe`, or a purpose-built test app.
- Do not use `git push --force` for release backup.
