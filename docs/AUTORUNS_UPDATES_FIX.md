# Autoruns and updates investigation

Base main: 0c23fe212eb51d2df634aceda215e1f742f1e1e7. Previous latest release: v0.1.14-rc.1; latest stable: v0.1.13.

## IFEO root cause

The original reader opened the base key with KEY_ENUMERATE_SUB_KEYS, then called RegQueryInfoKeyW, which requires KEY_QUERY_VALUE. This causes ERROR_ACCESS_DENIED even when the account can read the registry key. The Windows integration test reproduces this exact handle-rights error in an isolated fixture. The new shared reader requests both required rights, reports actual returned statuses at every stage and preserves partial results. Missing optional values and missing locations are normal, separate from permission failures. It does not request write access or modify IFEO. Read-only entries stay read-only.

Microsoft references:
- https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regqueryinfokeyw
- https://learn.microsoft.com/en-us/windows/win32/winprog64/shared-registry-keys
- https://learn.microsoft.com/en-us/windows/win32/winprog64/accessing-an-alternate-registry-view

## Update path

The manual-check handler already called checkForUpdates(true), but the implementation requested /releases/latest, which excludes pre-releases. Therefore a newer v0.1.14-rc.1 could not be offered from v0.1.13. The comparator also discarded RC identifiers, the manual flag did not override ignored versions, modal exceptions were swallowed, the first EXE/ZIP asset was chosen indiscriminately, and Portable ZIP was fed into the native downloader's hard-coded .exe staging path. A stored automatic-install preference could begin a download without a button press.

The shared update service selects the explicit stable or test channel, compares full RC identity, validates WPCC release asset URLs and reports API/network errors. The app opens the modal only for a newer release. Manual checks override ignore; automatic checks respect it. The modal displays notes and separate Setup/Portable guidance. Setup download and installation are separate explicit actions. Portable opens its own ZIP URL in the browser and never enters the installer downloader. Native Setup downloads reject non-Setup assets and concurrent requests. External links use the existing native browser-opening message and surface launch errors.

## Validation scope

Windows CTest includes registry_views and update_service, plus existing startup_transaction and settings_queue tests, in Debug and Release. UI integration runs in Chromium against the actual frontend with simulated host/API responses. Screenshots at 1280x720 and 900x600 are uploaded by Actions. Successful browser tests are not native WebView2 or real-user desktop tests. Release notes state the remaining manual checks.

## Completed Windows CI validation

[Actions run 37455135875](https://github.com/Zayoooh1/WindowsProcessControlCenter/actions/runs/37455135875) passed at commit b889483f7c52af7bf3ec69e4ddb5ce3c49b9c692. Both configurations passed all four CTest suites. Chromium integration tests passed, and the uploaded screenshots were reviewed at 1280x720 and 900x600. Package verification passed for versions, manifest, matching Portable payload, silent Setup installation and clean uninstall. Publication rebuilds and retests the final documentation commit before attaching its packages.
