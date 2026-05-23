#include "app/Application.h"

#include <Windows.h>

#include <cstdlib>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    wpcc::Application app(instance, showCommand);
    if (!app.Initialize())
    {
        MessageBoxW(nullptr, L"Windows Process Control Center failed to initialize.", L"Startup error", MB_ICONERROR | MB_OK);
        return EXIT_FAILURE;
    }

    return app.Run();
}
