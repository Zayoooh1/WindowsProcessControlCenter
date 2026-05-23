# GUI Library Decision

## Decision

Windows Process Control Center uses **WebView2** as the active UI layer.

The application remains a native Windows desktop program:

- Win32 creates and owns the native window.
- WebView2 renders the local HTML/CSS/JavaScript interface from `web/`.
- C++/WinAPI remains responsible for process enumeration and guarded process actions.
- JavaScript communicates with C++ through the WebView2 message bridge.

## Why WebView2

- It gives the project a more modern, flexible UI than the earlier immediate-mode prototype.
- The frontend can be styled with ordinary HTML/CSS without adding React, Vite, or a frontend build chain.
- Local assets are easy to package beside the executable for a portable release.
- The native backend can stay focused on WinAPI process logic and safety checks.

## Alternatives Considered

- **Dear ImGui**: fast for prototypes and diagnostics, but required too much custom styling to avoid a technical debug-tool feel.
- **Qt 6 Widgets**: mature desktop toolkit, but adds a larger runtime and deployment model.
- **Qt 6 QML**: strong visual potential, but adds QML/runtime complexity that is not needed yet.
- **WinUI 3 / Windows App SDK**: most native modern Windows look, but would require a larger packaging and project-structure shift.

## Current Recommendation

Stay on WebView2 for the active UI. Revisit WinUI 3 or Qt only if the project later needs deeper native accessibility, platform controls, or a more traditional installer-based deployment model.
