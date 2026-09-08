# Windows Process Control Center (WPCC)

Windows Process Control Center is a lightweight Windows utility for inspecting and controlling running processes, managing per-application preferences, reviewing startup entries, and monitoring essential system activity from one interface.

## What is WPCC?

WPCC combines a native C++ backend with a local WebView2 interface. It provides practical process and startup controls without treating every application as an optimization target or promising universal performance gains.

Process snapshots stay lightweight, while details and supported controls are loaded on demand. Administrator privileges are requested because actions such as changing process settings, controlling services, and inspecting protected system areas require elevated access.

## Features

### Process Control

- Inspect running processes and detailed per-process information.
- Change CPU priority and CPU affinity where Windows permits it.
- Select detected performance cores on supported hybrid CPUs.
- Suspend, resume, or end a process with safeguards for critical system processes.
- Show or hide likely Windows system processes in the process list.
- Pin frequently used process names as favorites.

### Profiles & Automation

- Create persistent profiles for individual applications.
- Match applications by executable path or process family.
- Automatically apply configured settings when matching applications run.
- Configure GPU preference, CPU priority, and CPU affinity where supported.
- Choose target executables through the native Windows file picker.

### Dashboard

- View process and access-status statistics from the current snapshot.
- Monitor total CPU and physical memory usage.
- Follow lightweight CPU and RAM history graphs while the Dashboard is open.

### Autoruns

WPCC currently displays these startup categories:

- Everything
- Logon
- Scheduled Tasks
- Services
- Drivers
- Explorer
- Winlogon
- AppInit
- Image Hijacks
- Known DLLs

`Everything` contains entries that WPCC can currently enable or disable safely. Sensitive or otherwise unsupported entries remain visible in their category as read-only information.

Autoruns supports search, sortable columns, category filtering, and optional **Hide Microsoft entries** and **Hide Windows entries** filters. These filters operate on the loaded snapshot. Refresh is explicit and on demand; WPCC does not continuously rescan Autoruns in the background.

### Safety & Architecture

- Native C++20 backend for Windows process and system operations.
- Local HTML, CSS, and JavaScript interface hosted with Microsoft Edge WebView2.
- Lightweight process snapshots with lazy, per-process detail loading.
- Autoruns scans only on demand; optional process-list refresh is user-configurable.
- Confirmation and protection checks around destructive or high-risk actions.
- Read-only treatment for sensitive Autoruns sources that WPCC cannot safely manage.

## Getting Started

1. Open the repository's [GitHub Releases](../../releases) page.
2. Download either:
   - `WindowsProcessControlCenter-vX.Y.Z-Setup.exe` for a standard installation.
   - `WindowsProcessControlCenter-vX.Y.Z-Portable.zip` for a portable copy.
3. Run WPCC and accept the administrator prompt when Windows displays it.

## System Requirements

- Windows 10 or Windows 11, x64.
- Microsoft Edge WebView2 Runtime.
- Administrator privileges for system-level functionality.

## Known Limitations

- Windows may restrict details or actions for protected processes even when WPCC is elevated.
- GPU preference changes generally apply the next time the target application starts.
- CPU affinity and performance-core controls depend on the processor topology and access rights reported by Windows.
- Autoruns is intentionally focused on the categories listed above and does not provide full Sysinternals Autoruns parity. Some sensitive categories are deliberately read-only.

## Development / Building

Requirements for a local build:

- CMake 3.24 or newer.
- Visual Studio 2022 with the Desktop development with C++ workload.
- An internet connection during initial configuration to obtain the pinned WebView2 SDK and JSON dependency.

Configure and build an x64 Release version:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The executable and synchronized `web` assets are written under `build\Release`.
