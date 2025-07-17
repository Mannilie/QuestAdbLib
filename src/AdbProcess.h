#pragma once

#include "../include/QuestAdbLib/Types.h"
#include "Utils.h"
#include <thread>
#include <mutex>
#include <functional>

namespace QuestAdbLib {

    using CompletionCallback = std::function<void(bool success, const std::string& output, const std::string& error)>;

    class AdbProcess {
    public:
        AdbProcess();
        ~AdbProcess();
        
        bool start(const std::string& command, int timeoutSeconds = 30, ProgressCallback progressCallback = nullptr);
        void stop();
        bool isRunning() const;
        bool wait(int timeoutSeconds = 0); // 0 = wait indefinitely
        
        Utils::ProcessResult getResult() const;
        void setCompletionCallback(CompletionCallback callback);
        
    private:
        std::thread thread_;
        mutable std::mutex mutex_;
        std::string command_;
        int timeoutSeconds_;
        ProgressCallback progressCallback_;
        CompletionCallback completionCallback_;
        Utils::ProcessResult result_;
        bool running_;
    };

} // namespace QuestAdbLib