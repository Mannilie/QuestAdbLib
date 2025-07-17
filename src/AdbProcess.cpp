#include "AdbProcess.h"
#include "Utils.h"
#include <chrono>
#include <thread>

using namespace std;

namespace QuestAdbLib {

    AdbProcess::AdbProcess() : running_(false) {}

    AdbProcess::~AdbProcess() { stop(); }

    bool AdbProcess::start(const string& command, int timeoutSeconds,
                           ProgressCallback progressCallback) {
        if (running_) {
            return false;
        }

        command_ = command;
        timeoutSeconds_ = timeoutSeconds;
        progressCallback_ = progressCallback;
        running_ = true;

        thread_ = thread([this]() {
            auto result = Utils::executeCommand(command_, timeoutSeconds_, progressCallback_);

            lock_guard<mutex> lock(mutex_);
            result_ = result;
            running_ = false;

            if (completionCallback_) {
                completionCallback_(result.success, result.output, result.error);
            }
        });

        return true;
    }

    void AdbProcess::stop() {
        if (running_) {
            // Note: This is a simplified implementation
            // In a full implementation, you would need to properly terminate the process
            running_ = false;
            if (thread_.joinable()) {
                thread_.join();
            }
        }
    }

    bool AdbProcess::isRunning() const {
        lock_guard<mutex> lock(mutex_);
        return running_;
    }

    bool AdbProcess::wait(int timeoutSeconds) {
        if (!thread_.joinable()) {
            return false;
        }

        if (timeoutSeconds <= 0) {
            thread_.join();
            return true;
        }

        auto start = chrono::steady_clock::now();
        while (running_ &&
               (chrono::steady_clock::now() - start) < chrono::seconds(timeoutSeconds)) {
            this_thread::sleep_for(chrono::milliseconds(100));
        }

        if (running_) {
            return false; // Timeout
        }

        if (thread_.joinable()) {
            thread_.join();
        }

        return true;
    }

    Utils::ProcessResult AdbProcess::getResult() const {
        lock_guard<mutex> lock(mutex_);
        return result_;
    }

    void AdbProcess::setCompletionCallback(CompletionCallback callback) {
        lock_guard<mutex> lock(mutex_);
        completionCallback_ = callback;
    }

} // namespace QuestAdbLib