# WPCC 0.1.15-rc.1: Autoruns and update fixes

This is a pre-release. Automated Windows Debug and Release builds, all four CTest suites in both configurations, browser integration tests and package verification passed. Real desktop use on Windows 10 and Windows 11 has not been verified. Browser UI tests use a simulated native host and simulated GitHub responses.

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
