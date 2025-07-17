#pragma once

#include "AdbCommand.h"
#include "Export.h"
#include "Types.h"
#include <chrono>
#include <memory>
#include <string>

namespace QuestAdbLib {

    class QUESTADBLIB_API AdbDevice {
      public:
        AdbDevice(const std::string& deviceId, std::shared_ptr<AdbCommand> adbCommand);
        ~AdbDevice();

        // Device information
        const std::string& getDeviceId() const { return deviceId_; }
        Result<DeviceInfo> getDeviceInfo();
        Result<std::string> getModel();
        Result<int> getBatteryLevel();
        Result<std::vector<std::string>> getRunningApps();

        // Device control
        Result<bool> reboot();
        Result<bool> waitForDevice(int timeoutSeconds = 60);
        Result<bool> applyConfiguration(const HeadsetConfig& config);

        // Shell operations
        Result<std::string> shell(const std::string& command, bool capture = true);
        Result<bool> setProperty(const std::string& property, const std::string& value);
        Result<std::string> getProperty(const std::string& property);

        // File operations
        Result<bool> pushFile(const std::string& localPath, const std::string& remotePath);
        Result<bool> pullFile(const std::string& remotePath, const std::string& localPath);
        Result<bool> removeFile(const std::string& remotePath);
        Result<bool> fileExists(const std::string& remotePath);

        // Broadcasting
        Result<bool> sendBroadcast(const std::string& action, const std::string& component = "");

        // Metrics operations
        Result<bool> startMetricsRecording();
        Result<bool> stopMetricsRecording();
        Result<bool> clearMetricsFiles();
        Result<std::vector<std::string>> getMetricsFiles();
        Result<std::string> pullLatestMetrics(const std::string& localDirectory);

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
        Result<bool> isAppRunning(const std::string& packageName);
        Result<bool> hasMetricsTriggerApps(const std::vector<std::string>& triggerApps);

      private:
        std::string deviceId_;
        std::shared_ptr<AdbCommand> adbCommand_;

        static constexpr const char* DEVICE_METRICS_PATH =
            "/sdcard/Android/data/com.oculus.ovrmonitormetricsservice/files/CapturedMetrics";
        static constexpr const char* METRICS_SERVICE_COMPONENT =
            "com.oculus.ovrmonitormetricsservice/.SettingsBroadcastReceiver";
    };

} // namespace QuestAdbLib