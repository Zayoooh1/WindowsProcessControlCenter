#pragma once

#include "core/ProcessActions.h"

#include <string>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace wpcc
{
    struct AutoApplyLog
    {
        std::string timestamp;
        std::string processName;
        std::string action;
        std::string status;
    };

    class AutoApplyEngine
    {
    public:
        AutoApplyEngine();
        ~AutoApplyEngine();

        void Start();
        void Stop();

        std::vector<AutoApplyLog> GetLogs() const;

    private:
        void ThreadLoop();
        void AddLog(const std::string& processName, const std::string& action, const std::string& status);

        std::thread m_thread;
        std::condition_variable m_cv;
        mutable std::mutex m_mutex;
        bool m_running = false;

        std::deque<AutoApplyLog> m_logs;
        const size_t MAX_LOGS = 100;
        ProcessActions m_processActions;
    };
}
