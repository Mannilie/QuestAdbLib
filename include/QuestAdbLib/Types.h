#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace QuestAdbLib {

    // Result type for operations
    template <typename T> struct Result {
        bool success;
        T value;
        std::string error;

        Result(bool s, T v, const std::string& e = "")
            : success(s), value(std::move(v)), error(e) {}

        static Result Success(T value) { return Result(true, std::move(value)); }

        static Result Error(const std::string& error) { return Result(false, T{}, error); }

        explicit operator bool() const { return success; }
    };

    // Device information
    struct DeviceInfo {
        std::string deviceId;
        std::string status; // "device", "unauthorized", "offline", etc.
        std::string model;
        int batteryLevel = -1;
        std::chrono::system_clock::time_point lastUpdated;
        std::vector<std::string> runningApps;

        DeviceInfo() = default;
        DeviceInfo(const std::string& id, const std::string& s)
            : deviceId(id), status(s), lastUpdated(std::chrono::system_clock::now()) {}
    };

    // Progress callback type
    using ProgressCallback = std::function<void(const std::string&)>;

    // ADB command execution options
    struct CommandOptions {
        bool captureOutput = true;
        int timeoutSeconds = 30;
        ProgressCallback progressCallback = nullptr;

        CommandOptions() = default;
        CommandOptions(bool capture, int timeout = 30)
            : captureOutput(capture), timeoutSeconds(timeout) {}
    };

    // Metrics session information
    struct MetricsSession {
        std::string deviceId;
        std::chrono::system_clock::time_point startTime;
        std::chrono::seconds duration;
        bool isRecording = false;
        std::string filePath;

        MetricsSession() = default;
        MetricsSession(const std::string& id, std::chrono::seconds dur)
            : deviceId(id), startTime(std::chrono::system_clock::now()), duration(dur) {}
    };

    // VR headset configuration
    struct HeadsetConfig {
        int cpuLevel = 4;
        int gpuLevel = 4;
        bool disableProximity = true;
        bool disableGuardian = false;
        int bootTimeoutSeconds = 60;
        int waitTimeoutSeconds = 15;
        std::chrono::seconds testDuration = std::chrono::seconds(30);

        HeadsetConfig() = default;
    };

} // namespace QuestAdbLib