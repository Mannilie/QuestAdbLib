#include "AdbProcess.h"
#include "Utils.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace QuestAdbLib {

AdbProcess::AdbProcess() : running_(false) {}

AdbProcess::~AdbProcess() {
    stop();
}

bool AdbProcess::start(const std::string& command, int timeoutSeconds, ProgressCallback progressCallback) {
    if (running_) {
        return false;
    }
    
    command_ = command;
    timeoutSeconds_ = timeoutSeconds;
    progressCallback_ = progressCallback;
    running_ = true;
    
    thread_ = std::thread([this]() {
        auto result = Utils::executeCommand(command_, timeoutSeconds_, progressCallback_);
        
        std::lock_guard<std::mutex> lock(mutex_);
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
    std::lock_guard<std::mutex> lock(mutex_);
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
    
    auto start = std::chrono::steady_clock::now();
    while (running_ && (std::chrono::steady_clock::now() - start) < std::chrono::seconds(timeoutSeconds)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    std::lock_guard<std::mutex> lock(mutex_);
    return result_;
}

void AdbProcess::setCompletionCallback(CompletionCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    completionCallback_ = callback;
}

} // namespace QuestAdbLib