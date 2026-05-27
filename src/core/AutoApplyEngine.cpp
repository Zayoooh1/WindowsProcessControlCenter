#include "core/AutoApplyEngine.h"
#include "core/ProfileStore.h"
#include "core/ProcessProvider.h"
#include "core/GpuPreferenceManager.h"

#include <Windows.h>
#include <algorithm>
#include <iostream>
#include <chrono>

namespace
{
    std::string NormalizeProcessName(std::string_view name)
    {
        std::string result(name);
        result.erase(0, result.find_first_not_of(" \t\r\n"));
        result.erase(result.find_last_not_of(" \t\r\n") + 1);
        for (char& c : result)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return result;
    }

    std::string NormalizePath(std::string_view path)
    {
        std::string result(path);
        result.erase(0, result.find_first_not_of(" \t\r\n"));
        result.erase(result.find_last_not_of(" \t\r\n") + 1);
        for (char& c : result)
        {
            if (c == '\\') c = '/';
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return result;
    }

    bool MatchProcessPath(std::string_view processPath, std::string_view targetPath)
    {
        std::string normProc = NormalizePath(processPath);
        std::string normTarget = NormalizePath(targetPath);
        return !normProc.empty() && !normTarget.empty() && normProc == normTarget;
    }

    bool MatchProcessName(std::string_view processName, std::string_view targetName)
    {
        std::string normProc = NormalizeProcessName(processName);
        std::string normTarget = NormalizeProcessName(targetName);
        if (normProc.empty() || normTarget.empty()) return false;
        if (normProc == normTarget) return true;
        if (normTarget.size() < 4 || normTarget.substr(normTarget.size() - 4) != ".exe")
        {
            if (normProc == normTarget + ".exe")
            {
                return true;
            }
        }
        return false;
    }

    std::string CurrentTimestamp()
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
        return std::string(buffer);
    }
}

namespace wpcc
{
    AutoApplyEngine::AutoApplyEngine() = default;

    AutoApplyEngine::~AutoApplyEngine()
    {
        Stop();
    }

    void AutoApplyEngine::Start()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_running)
        {
            return;
        }

        m_running = true;
        m_thread = std::thread(&AutoApplyEngine::ThreadLoop, this);
    }

    void AutoApplyEngine::Stop()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_running)
            {
                return;
            }
            m_running = false;
        }
        m_cv.notify_all();

        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    std::vector<AutoApplyLog> AutoApplyEngine::GetLogs() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::vector<AutoApplyLog>(m_logs.begin(), m_logs.end());
    }

    void AutoApplyEngine::AddLog(const std::string& processName, const std::string& action, const std::string& status)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        AutoApplyLog log;
        log.timestamp = CurrentTimestamp();
        log.processName = processName;
        log.action = action;
        log.status = status;

        m_logs.push_front(log);
        if (m_logs.size() > MAX_LOGS)
        {
            m_logs.pop_back();
        }
    }

    void AutoApplyEngine::ThreadLoop()
    {
        ProcessProvider processProvider;
        GpuPreferenceManager gpuManager;

        while (true)
        {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (m_cv.wait_for(lock, std::chrono::seconds(5), [this]() { return !m_running; }))
                {
                    break;
                }
            }

            // Fetch profiles
            ProfileLoadResult loadResult = ProfileStore::GetProfiles();
            if (!loadResult.success) continue;

            std::vector<Profile> profiles = ProfileStore::ParseProfilesJson(loadResult.jsonContent);
            std::vector<Profile> activeProfiles;
            for (const auto& p : profiles)
            {
                if (p.autoApply) activeProfiles.push_back(p);
            }

            if (activeProfiles.empty()) continue;

            // Fetch processes
            std::vector<ProcessInfo> processes = processProvider.LoadProcesses();

            // Match and apply
            for (const Profile& profile : activeProfiles)
            {
                for (ProcessInfo& process : processes)
                {
                    bool isMatch = false;
                    if (profile.matchMode == "path")
                    {
                        if (!process.executablePath.empty())
                        {
                            isMatch = MatchProcessPath(process.executablePath, profile.targetExePath);
                        }
                    }
                    else if (profile.matchMode == "name")
                    {
                        isMatch = MatchProcessName(process.name, profile.targetProcessName);
                    }

                    if (isMatch)
                    {
                        // Check CPU Priority
                        if (profile.cpuPriority != "DoNotChange" && !profile.cpuPriority.empty())
                        {
                            if (process.cpuPriority != profile.cpuPriority)
                            {
                                // We need to normalize priority strings to avoid infinite loops, SetCpuPriority handles priority string resolution.
                                // Actually ProcessInfo returns formatted "Normal", "High" etc.
                                // We check if it differs.
                                std::string currentNorm = process.cpuPriority;
                                std::string targetNorm = profile.cpuPriority;
                                // Simple normalization for comparison
                                auto normalize = [](std::string& s) {
                                    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
                                    for(auto& c: s) c = std::tolower(c);
                                };
                                normalize(currentNorm);
                                normalize(targetNorm);

                                if (currentNorm != targetNorm)
                                {
                                    ProcessActionResult result = m_processActions.SetCpuPriority(process.pid, profile.cpuPriority, profile.allowRealtime);
                                    if (result.success)
                                    {
                                        AddLog(process.name, "CPU: " + profile.cpuPriority, "Success");
                                        process.cpuPriority = profile.cpuPriority; // Update local state so we don't try again
                                    }
                                    else if (result.win32ErrorCode != ERROR_ACCESS_DENIED) // don't spam access denied
                                    {
                                        AddLog(process.name, "CPU: " + profile.cpuPriority, "Failed: " + result.message);
                                    }
                                }
                            }
                        }

                        // Check GPU Preference
                        if (profile.gpuPreference != "DoNotChange" && !profile.gpuPreference.empty())
                        {
                            process.gpuPreference = gpuManager.GetPreferenceForExecutablePath(process.executablePath);
                            
                            std::string currentNorm = process.gpuPreference;
                            std::string targetNorm = profile.gpuPreference;
                            auto normalize = [](std::string& s) {
                                s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
                                for(auto& c: s) c = std::tolower(c);
                            };
                            normalize(currentNorm);
                            normalize(targetNorm);

                            if (currentNorm != targetNorm)
                            {
                                ProcessActionResult result = gpuManager.SetPreferenceForProcess(process.pid, process.name, process.executablePath, profile.gpuPreference);
                                if (result.success)
                                {
                                    AddLog(process.name, "GPU: " + profile.gpuPreference, "Success");
                                    process.gpuPreference = profile.gpuPreference; // Update local state
                                }
                                else if (result.win32ErrorCode != ERROR_ACCESS_DENIED)
                                {
                                    AddLog(process.name, "GPU: " + profile.gpuPreference, "Failed: " + result.message);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
