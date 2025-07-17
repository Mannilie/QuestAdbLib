#pragma once

#include "../include/QuestAdbLib/Types.h"
#include "Utils.h"
#include <functional>
#include <mutex>
#include <thread>

using namespace std;

namespace QuestAdbLib {

    using CompletionCallback =
        function<void(bool success, const string& output, const string& error)>;

    class AdbProcess {
      public:
        AdbProcess();
        ~AdbProcess();

        bool start(const string& command, int timeoutSeconds = 30,
                   ProgressCallback progressCallback = nullptr);
        void stop();
        bool isRunning() const;
        bool wait(int timeoutSeconds = 0); // 0 = wait indefinitely

        Utils::ProcessResult getResult() const;
        void setCompletionCallback(CompletionCallback callback);

      private:
        thread thread_;
        mutable mutex mutex_;
        string command_;
        int timeoutSeconds_;
        ProgressCallback progressCallback_;
        CompletionCallback completionCallback_;
        Utils::ProcessResult result_;
        bool running_;
    };

} // namespace QuestAdbLib