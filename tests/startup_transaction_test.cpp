#include "core/StartupTransaction.h"
#ifdef NDEBUG
#undef NDEBUG // Keep checks active in Release/CTest too.
#endif
#include <cassert>
#include <iostream>
struct State { bool known = true, enabled = false, legacy = false; long error = 0; std::wstring warning; };
struct Save { bool success; std::wstring warning; };
int main()
{
    int reads = 0, writes = 0, saves = 0;
    State system;
    auto read = [&] { ++reads; return system; };
    auto configure = [&](bool enabled) { ++writes; system.enabled = enabled; system.legacy = false; return system; };
    auto persist = [&] { ++saves; return Save{true, L""}; };
    auto result = wpcc::SaveStartupSettings(true, true, read, configure, persist);
    assert(result.success && result.startup.enabled && writes == 1 && saves == 1);
    result = wpcc::SaveStartupSettings(true, false, read, configure, persist);
    assert(result.success && !result.startup.enabled);
    result = wpcc::SaveStartupSettings(true, false, read, configure, persist);
    assert(result.success && !result.startup.enabled); // repeat disable
    const int before = writes;
    result = wpcc::SaveStartupSettings(false, true, read, configure, persist);
    assert(result.success && !result.startup.enabled && writes == before && result.warning.find(L"differs from Windows") != std::wstring::npos); // stale intent cannot re-enable
    auto denied = [&](bool) { return State{true, false, false, -1, L"access denied"}; };
    int savedBefore = saves;
    result = wpcc::SaveStartupSettings(true, true, read, denied, persist);
    assert(!result.success && saves == savedBefore && result.warning == L"access denied");
    auto unknown = [&](bool) { return State{false, false, false, -1, L"scheduler unavailable"}; };
    result = wpcc::SaveStartupSettings(true, true, read, unknown, persist);
    assert(!result.success && saves == savedBefore);
    auto diskFailure = [&] { ++saves; return Save{false, L"disk full"}; };
    result = wpcc::SaveStartupSettings(true, true, read, configure, diskFailure);
    assert(!result.success && result.startup.enabled && result.warning.find(L"Windows startup changed") != std::wstring::npos);
    system.legacy = true;
    auto cleanupFailure = [&](bool enabled) { system.enabled = enabled; return system; };
    savedBefore = saves;
    result = wpcc::SaveStartupSettings(true, true, read, cleanupFailure, persist);
    assert(!result.success && result.startup.legacy && saves == savedBefore);
    result = wpcc::SaveStartupSettings(true, true, read, configure, persist);
    assert(result.success && !result.startup.legacy); // migration confirmed before persistence
    system.known = false; system.warning = L"scheduler unavailable";
    result = wpcc::SaveStartupSettings(false, false, read, configure, persist);
    assert(result.success && result.warning == L"scheduler unavailable"); // unrelated settings remain usable
    assert(reads >= 3);
    std::cout << "10 startup transaction scenarios passed\n";
}
