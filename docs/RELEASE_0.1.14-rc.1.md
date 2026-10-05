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
