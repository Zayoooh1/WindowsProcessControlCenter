#include "core/SettingsService.h"
#include "core/StartupTransaction.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace wpcc
{
    using nlohmann::json;
    SettingsUpdateResult SettingsService::Load()
    {
        auto stored = SettingsStore::GetSettings();
        SettingsUpdateResult result;
        result.success = stored.success; result.warning = stored.warning;
        result.startup = StartupManager::Read();
        try
        {
            auto data = stored.success ? json::parse(stored.jsonContent) : json::object();
            if (!data.is_object()) throw std::runtime_error("Settings must be an object");
            const bool intent = data.value("startWithWindows", false);
            data["startWithWindows"] = result.startup.known && result.startup.enabled;
            result.jsonContent = data.dump();
            if (result.startup.known && intent != result.startup.enabled)
                result.warning += L" Saved startup preference differs from Windows. The switch shows Windows configuration; change it explicitly to reconcile. ";
            result.warning += result.startup.warning;
        }
        catch (...) { result.success = false; result.warning += L" Invalid settings object; original file was preserved."; }
        return result;
    }
    SettingsUpdateResult SettingsService::Save(const std::string& requested, bool changeStartup)
    {
        auto old = SettingsStore::GetSettings();
        if (!old.success) return {false, "", old.warning, StartupManager::Read()};
        try
        {
            auto data = json::parse(old.jsonContent);
            auto update = json::parse(requested);
            if (!data.is_object() || !update.is_object()) throw std::runtime_error("Settings must be an object");
            for (const auto* key : {"startWithWindows", "minimizeToTray"})
                if (update.contains(key) && !update[key].is_boolean()) throw std::runtime_error("Invalid boolean setting");
            const bool oldIntent = data.value("startWithWindows", false);
            data.update(update); // Preserve fields unknown to this frontend.
            if (!changeStartup) data["startWithWindows"] = oldIntent;
            const bool desired = data.value("startWithWindows", false);
            auto saved = SaveStartupSettings(changeStartup, desired,
                [] { return StartupManager::Read(); },
                [](bool enabled) { return StartupManager::SetEnabled(enabled); },
                [&] { return SettingsStore::SaveSettings(data.dump()); });
            SettingsUpdateResult result;
            result.success = saved.success; result.warning = saved.warning; result.startup = saved.startup;
            if (!saved.success) data = json::parse(old.jsonContent);
            data["startWithWindows"] = result.startup.known && result.startup.enabled;
            result.jsonContent = data.dump();
            StartupLog(L"settings.save", saved.success ? S_OK : E_FAIL);
            return result;
        }
        catch (...) { return {false, old.jsonContent, L"Settings must be a JSON object with boolean startup/tray fields. Nothing was saved.", StartupManager::Read()}; }
    }
}
