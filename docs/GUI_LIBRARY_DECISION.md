# GUI Library Decision

## Context

Windows Process Control Center is currently a C++20 Win32 desktop application using DirectX 11 and Dear ImGui. The app already has a read-only Windows process snapshot provider and a custom dark UI shell. This document evaluates whether the project should remain on Dear ImGui or migrate to another GUI stack.

## Dear ImGui

Plusy:
- Very fast to integrate with the existing Win32 and DirectX 11 project.
- Excellent for dense tools, tables, diagnostics, and process-manager workflows.
- Small dependency surface and simple CMake integration through `FetchContent`.
- Easy to keep all process logic in native C++ without bridging.

Minusy:
- Requires deliberate custom styling to avoid a stock technical/debug-tool look.
- Accessibility, native controls, localization, and advanced layout behavior require extra work.
- Complex app flows can become harder to maintain without careful UI component structure.

Build impact:
- Low. Current CMake setup already downloads and builds it.

Visual impact:
- Good enough for a polished utility if the theme and custom components continue to mature.
- Less naturally native than WinUI or Qt without ongoing design work.

Integration difficulty:
- Already integrated. Lowest risk option.

Migrate now or later:
- Do not migrate now. Continue with Dear ImGui for the next read-only and safety-focused stages.

## Qt 6 Widgets

Plusy:
- Mature desktop widgets, strong tables, menus, dialogs, settings, and layouts.
- Good fit for classic desktop utilities and administrative tools.
- Better built-in layout behavior than immediate-mode UI.

Minusy:
- Larger dependency and deployment footprint.
- Requires a meaningful rewrite of the UI layer.
- Styling can still look dated unless a custom theme is built.

Build impact:
- Medium to high. Requires Qt installation, CMake package discovery, deployment tooling, and runtime DLL handling.

Visual impact:
- Can look professional, but a modern dark theme needs custom styling.

Integration difficulty:
- Medium. Native C++ integration is good, but the current ImGui UI would need to be replaced.

Migrate now or later:
- Consider later if the project grows into a settings-heavy multi-window desktop application.

## Qt 6 QML

Plusy:
- Strong modern UI potential with animations, responsive layouts, and custom components.
- Better suited than Widgets for highly polished visual design.
- Good separation between UI and backend if architected carefully.

Minusy:
- Adds QML runtime, a new language layer, and more build/deploy complexity.
- Requires bridging C++ process data into QML models.
- Can be overkill for a focused process manager.

Build impact:
- High. Requires Qt Quick/QML modules, resource handling, deployment, and model binding work.

Visual impact:
- Very strong if the team invests in custom QML components and visual QA.

Integration difficulty:
- Medium to high. Native C++ backend is fine, but UI migration would be substantial.

Migrate now or later:
- Consider later only if the app needs a highly animated consumer-grade interface.

## WinUI 3 / Windows App SDK

Plusy:
- Most native modern Windows look.
- Good typography, accessibility, high DPI behavior, and modern controls.
- Strong long-term fit for a Windows-only desktop application.

Minusy:
- More complex packaging and Windows App SDK dependency model.
- Integration with existing raw Win32/DX11/ImGui shell would require a major UI rewrite.
- Some desktop/system-tool scenarios can require careful interop.

Build impact:
- High. Requires Windows App SDK setup, packaging choices, and different project structure.

Visual impact:
- Excellent native Windows 11 style out of the box.

Integration difficulty:
- High from the current codebase.

Migrate now or later:
- Consider later if native Windows polish and accessibility become the top priority.

## WebView2 + HTML/CSS Frontend

Plusy:
- Best styling flexibility with HTML/CSS.
- Easy to create modern layouts, responsive panels, and rich visual components.
- Large UI ecosystem.

Minusy:
- Requires a native-to-web bridge for process data and actions.
- Adds WebView2 runtime dependency and frontend build tooling if using a framework.
- More moving parts for a native system utility.

Build impact:
- Medium to high depending on whether plain HTML/CSS or a frontend framework is used.

Visual impact:
- Excellent visual flexibility, but can feel less native if not designed carefully.

Integration difficulty:
- Medium. Process backend can remain C++, but UI communication must be designed and secured.

Migrate now or later:
- Consider later if the project prioritizes very rich UI design over native simplicity.

## Updated Recommendation After Task 05

The project is moving from Dear ImGui to WebView2. The reason is visual quality: the process manager needs a more polished, product-grade desktop UI than the current immediate-mode shell can provide without substantial custom drawing work.

The backend remains native C++/WinAPI. `ProcessProvider` continues to enumerate real Windows processes, while WebView2 hosts a local vanilla HTML/CSS/JavaScript frontend. C++ sends process snapshots to JavaScript through `PostWebMessageAsJson`, and JavaScript requests refreshes through `window.chrome.webview.postMessage`.

This is not a migration to a web app. It remains a native Windows desktop application with a WebView2-rendered interface.

Current recommendation:
- Use WebView2 as the active UI layer.
- Keep the old Dear ImGui files as legacy reference for now, excluded from the normal build.
- Avoid React/Vite or other frontend build stacks until the UI has enough complexity to justify them.
- Revisit WinUI 3 later only if native Windows accessibility and platform controls become more important than HTML/CSS visual flexibility.
