# Windows Process Control Center

Windows Process Control Center is a C++20 Win32 desktop application for Windows process inspection and controlled process-management workflows. The current UI is rendered with Microsoft Edge WebView2 using local vanilla HTML, CSS, and JavaScript, while the backend remains native C++/WinAPI.

The app can display real process snapshots and change the CPU priority class of accessible user processes. It does not terminate, freeze, resume, change GPU preference, or persist process settings yet.

## Requirements

- Windows 10 or newer
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

## Run

After a Debug build:

```powershell
.\build\Debug\WindowsProcessControlCenter.exe
```

After a Release build:

```powershell
.\build\Release\WindowsProcessControlCenter.exe
```

## Current Scope

Implemented so far:

- Win32 desktop application entry point and native window.
- WebView2 host for local frontend rendering.
- Vanilla HTML/CSS/JS frontend in `web/`.
- C++ to JavaScript message bridge for process snapshots.
- JavaScript to C++ message bridge for `refreshProcesses`.
- JavaScript to C++ message bridge for `setCpuPriority`.
- Read-only process enumeration using Windows APIs.
- CPU priority changes for accessible processes through `OpenProcess` and `SetPriorityClass`.
- Realtime priority safeguard requiring explicit frontend confirmation and backend validation.
- Process filtering by name, PID, or executable path.
- Process details panel with executable path, CPU priority, access status, admin requirement hint, and access error details when available.
- Disabled future action buttons for process-management functions.

Not implemented yet:

- Ending processes.
- Freezing or resuming processes.
- GPU preference changes.
- Settings persistence.
- Profiles, rules, autostart, elevation, or admin workflows.

## Project Structure

```text
src/
  app/        Application lifecycle and main loop.
  core/       Process model and read-only process provider.
  platform/   Win32 window wrapper.
  ui_web/     WebView2 host and message bridge.
  ui/         Legacy Dear ImGui UI kept in the repo but excluded from active build.
web/          Local vanilla HTML/CSS/JS frontend loaded by WebView2.
docs/         Project documentation.
```
