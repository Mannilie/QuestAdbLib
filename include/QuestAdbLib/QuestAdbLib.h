#pragma once

#include "AdbCommand.h"
#include "AdbDevice.h"
#include "Export.h"
#include "Types.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace QuestAdbLib {

    // Event callback types
    using DeviceStatusCallback =
        std::function<void(const std::string& deviceId, const std::string& status)>;
    using DeviceListCallback = std::function<void(const std::vector<DeviceInfo>&)>;
    using MetricsProgressCallback =
        std::function<void(const std::string& deviceId, double progress)>;

    class QUESTADBLIB_API QuestAdbManager {
      public:
        QuestAdbManager();
        explicit QuestAdbManager(const std::string& adbPath);
        ~QuestAdbManager();

        // Initialization
        Result<bool> initialize();
        bool isInitialized() const { return initialized_; }

        // Device management
        Result<std::vector<DeviceInfo>> getConnectedDevices();
        Result<std::shared_ptr<AdbDevice>> getDevice(const std::string& deviceId);
        Result<bool> refreshDeviceList();

        // Event handling
        void setDeviceStatusCallback(DeviceStatusCallback callback);
        void setDeviceListCallback(DeviceListCallback callback);
        void setMetricsProgressCallback(MetricsProgressCallback callback);

        // Monitoring
        Result<bool> startDeviceMonitoring(int intervalSeconds = 5);
        void stopDeviceMonitoring();
        bool isMonitoring() const { return monitoring_; }

        // Batch operations
        Result<bool> rebootAndWaitAll();
        Result<bool> applyConfigurationAll(const HeadsetConfig& config);
        Result<std::map<std::string, bool>> runCommandOnAll(const std::string& command);

        // Metrics operations
        Result<bool>
        startMetricsRecordingAll(std::chrono::seconds duration = std::chrono::seconds(30));
        Result<bool> stopMetricsRecordingAll();
        Result<std::map<std::string, std::string>>
        pullMetricsAll(const std::string& localDirectory);

        // Configuration
        void setDefaultConfiguration(const HeadsetConfig& config);
        const HeadsetConfig& getDefaultConfiguration() const;

        // Utility functions
        static std::string findAdbPath();
        static Result<bool> isAdbAvailable(const std::string& adbPath = "");
        static std::vector<std::string> getPlatformToolsPaths();

        // Library information
        static std::string getVersion();
        static std::string getBuildInfo();

      private:
        std::shared_ptr<AdbCommand> adbCommand_;
        std::map<std::string, std::shared_ptr<AdbDevice>> devices_;
        std::map<std::string, MetricsSession> activeSessions_;

        bool initialized_ = false;
        bool monitoring_ = false;
        HeadsetConfig defaultConfig_;

        // Callbacks
        DeviceStatusCallback deviceStatusCallback_;
        DeviceListCallback deviceListCallback_;
        MetricsProgressCallback metricsProgressCallback_;

        // Monitoring
        class MonitoringThread;
        std::unique_ptr<MonitoringThread> monitoringThread_;

        // Internal methods
        void updateDeviceList();
        void emitDeviceStatusChange(const std::string& deviceId, const std::string& status);
        void emitDeviceListUpdate(const std::vector<DeviceInfo>& devices);
        void emitMetricsProgress(const std::string& deviceId, double progress);

        // Disable copy and assignment
        QuestAdbManager(const QuestAdbManager&) = delete;
        QuestAdbManager& operator=(const QuestAdbManager&) = delete;
    };

} // namespace QuestAdbLib