#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

using namespace std;
using namespace std::chrono;

namespace QuestAdbLib {

    // Result type for operations
    template <typename T> struct Result {
        bool success;
        T value;
        string error;

        Result(bool s, T v, const string& e = "")
            : success(s), value(move(v)), error(e) {}

        static Result Success(T value) { return Result(true, move(value)); }

        static Result Error(const string& error) { return Result(false, T{}, error); }

        explicit operator bool() const { return success; }
    };

    // Device information
    struct DeviceInfo {
        string deviceId;
        string status; // "device", "unauthorized", "offline", etc.
        string model;
        int batteryLevel = -1;
        system_clock::time_point lastUpdated;
        vector<string> runningApps;

        DeviceInfo() = default;
        DeviceInfo(const string& id, const string& s)
            : deviceId(id), status(s), lastUpdated(system_clock::now()) {}
    };

    // Progress callback type
    using ProgressCallback = function<void(const string&)>;

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
        string deviceId;
        system_clock::time_point startTime;
        seconds duration;
        bool isRecording = false;
        string filePath;

        MetricsSession() = default;
        MetricsSession(const string& id, seconds dur)
            : deviceId(id), startTime(system_clock::now()), duration(dur) {}
    };

    // VR headset configuration
    struct HeadsetConfig {
        int cpuLevel = 4;
        int gpuLevel = 4;
        bool disableProximity = true;
        bool disableGuardian = false;
        int bootTimeoutSeconds = 60;
        int waitTimeoutSeconds = 15;
        seconds testDuration = seconds(30);

        HeadsetConfig() = default;
    };

} // namespace QuestAdbLib