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

using namespace std;

namespace QuestAdbLib {

    // Event callback types
    using DeviceStatusCallback =
        function<void(const string& deviceId, const string& status)>;
    using DeviceListCallback = function<void(const vector<DeviceInfo>&)>;
    using MetricsProgressCallback =
        function<void(const string& deviceId, double progress)>;

    class QUESTADBLIB_API QuestAdbManager {
      public:
        QuestAdbManager();
        explicit QuestAdbManager(const string& adbPath);
        ~QuestAdbManager();

        // Initialization
        Result<bool> initialize();
        bool isInitialized() const { return initialized_; }

        // Device management
        Result<vector<DeviceInfo>> getConnectedDevices();
        Result<shared_ptr<AdbDevice>> getDevice(const string& deviceId);
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
        Result<map<string, bool>> runCommandOnAll(const string& command);

        // Metrics operations
        Result<bool>
        startMetricsRecordingAll(chrono::seconds duration = chrono::seconds(30));
        Result<bool> stopMetricsRecordingAll();
        Result<map<string, string>>
        pullMetricsAll(const string& localDirectory);

        // Configuration
        void setDefaultConfiguration(const HeadsetConfig& config);
        const HeadsetConfig& getDefaultConfiguration() const;

        // Utility functions
        static string findAdbPath();
        static Result<bool> isAdbAvailable(const string& adbPath = "");
        static vector<string> getPlatformToolsPaths();

        // Library information
        static string getVersion();
        static string getBuildInfo();

      private:
        shared_ptr<AdbCommand> adbCommand_;
        map<string, shared_ptr<AdbDevice>> devices_;
        map<string, MetricsSession> activeSessions_;

        bool initialized_ = false;
        bool monitoring_ = false;
        HeadsetConfig defaultConfig_;

        // Callbacks
        DeviceStatusCallback deviceStatusCallback_;
        DeviceListCallback deviceListCallback_;
        MetricsProgressCallback metricsProgressCallback_;

        // Monitoring
        class MonitoringThread;
        unique_ptr<MonitoringThread> monitoringThread_;

        // Internal methods
        void updateDeviceList();
        void emitDeviceStatusChange(const string& deviceId, const string& status);
        void emitDeviceListUpdate(const vector<DeviceInfo>& devices);
        void emitMetricsProgress(const string& deviceId, double progress);

        // Disable copy and assignment
        QuestAdbManager(const QuestAdbManager&) = delete;
        QuestAdbManager& operator=(const QuestAdbManager&) = delete;
    };

} // namespace QuestAdbLib