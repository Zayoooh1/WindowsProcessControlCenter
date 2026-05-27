#include "app/Application.h"

#include <Windows.h>

#include <cstdlib>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmdLine, int showCommand)
{
    bool startMinimized = cmdLine && wcsstr(cmdLine, L"--minimized") != nullptr;
    wpcc::Application app(instance, showCommand, startMinimized);
    if (!app.Initialize())
    {
        MessageBoxW(nullptr, L"Windows Process Control Center failed to initialize.", L"Startup error", MB_ICONERROR | MB_OK);
        return EXIT_FAILURE;
    }

    return app.Run();
}
