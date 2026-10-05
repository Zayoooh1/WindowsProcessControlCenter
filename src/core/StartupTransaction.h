#pragma once
#include <string>
#include <utility>

namespace wpcc
{
    // Explicit partial-commit policy: never falsely report success after an OS or disk failure.
    // Keeping the re-read OS state is safer than a rollback that could also fail.
    template<class State> struct StartupTransactionResult
    {
        bool success = false;
        State startup;
        std::wstring warning;
    };
    template<class Read, class Configure, class Persist>
    auto SaveStartupSettings(bool changeStartup, bool desired, Read read, Configure configure, Persist persist)
    {
        using State = decltype(read());
        StartupTransactionResult<State> result;
        result.startup = changeStartup ? configure(desired) : read();
        if (changeStartup && (!result.startup.known || result.startup.error != 0 || result.startup.enabled != desired || result.startup.legacy))
        {
            result.warning = result.startup.warning;
            if (result.warning.empty()) result.warning = L"Windows did not confirm the requested startup configuration.";
            return result;
        }
        const auto saved = persist();
        result.success = saved.success;
        result.warning = saved.warning;
        if (!saved.success && changeStartup)
        {
            result.startup = read();
            result.warning += L" Windows startup changed, but settings.json was not saved. The switch shows the verified Windows state; retry saving.";
        }
        else if (!result.startup.warning.empty()) result.warning += result.startup.warning;
        if (saved.success && !changeStartup && result.startup.known && result.startup.enabled != desired)
            result.warning += L" Saved startup preference differs from Windows. Change startup explicitly to reconcile.";
        return result;
    }
}
