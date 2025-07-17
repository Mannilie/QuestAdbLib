#pragma once

#include "AdbCommand.h"
#include "Export.h"
#include "Types.h"
#include <chrono>
#include <memory>
#include <string>

using namespace std;

namespace QuestAdbLib {

    class QUESTADBLIB_API AdbDevice {
      public:
        AdbDevice(const string& deviceId, shared_ptr<AdbCommand> adbCommand);
        ~AdbDevice();

        // Device information
        const string& getDeviceId() const { return deviceId_; }
        Result<DeviceInfo> getDeviceInfo();
        Result<string> getModel();
        Result<int> getBatteryLevel();
        Result<vector<string>> getRunningApps();

        // Device control
        Result<bool> reboot();
        Result<bool> waitForDevice(int timeoutSeconds = 60);
        Result<bool> applyConfiguration(const HeadsetConfig& config);

        // Shell operations
        Result<string> shell(const string& command, bool capture = true);
        Result<bool> setProperty(const string& property, const string& value);
        Result<string> getProperty(const string& property);

        // File operations
        Result<bool> pushFile(const string& localPath, const string& remotePath);
        Result<bool> pullFile(const string& remotePath, const string& localPath);
        Result<bool> removeFile(const string& remotePath);
        Result<bool> fileExists(const string& remotePath);

        // Broadcasting
        Result<bool> sendBroadcast(const string& action, const string& component = "");

        // Metrics operations
        Result<bool> startMetricsRecording();
        Result<bool> stopMetricsRecording();
        Result<bool> clearMetricsFiles();
        Result<vector<string>> getMetricsFiles();
        Result<string> pullLatestMetrics(const string& localDirectory);

        // VR-specific operations
        Result<bool> setCpuLevel(int level);
        Result<bool> setGpuLevel(int level);
        Result<bool> disableProximity();
        Result<bool> disableGuardian();
        Result<bool> enableMetricsOverlay();
        Result<bool> disableMetricsOverlay();
        Result<bool> enableCsvMetrics();
        Result<bool> disableCsvMetrics();

        // Process management
        Result<bool> isAppRunning(const string& packageName);
        Result<bool> hasMetricsTriggerApps(const vector<string>& triggerApps);

      private:
        string deviceId_;
        shared_ptr<AdbCommand> adbCommand_;

        static constexpr const char* DEVICE_METRICS_PATH =
            "/sdcard/Android/data/com.oculus.ovrmonitormetricsservice/files/CapturedMetrics";
        static constexpr const char* METRICS_SERVICE_COMPONENT =
            "com.oculus.ovrmonitormetricsservice/.SettingsBroadcastReceiver";
    };

} // namespace QuestAdbLib