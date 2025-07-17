#include "../include/QuestAdbLib/QuestAdbLib.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

namespace QuestAdbLib {

    class QuestAdbManager::MonitoringThread {
      public:
        MonitoringThread(QuestAdbManager* manager) : manager_(manager), running_(false) {}

        ~MonitoringThread() { stop(); }

        void start(int intervalSeconds) {
            if (running_) {
                return;
            }

            running_ = true;
            thread_ = thread([this, intervalSeconds]() {
                while (running_) {
                    manager_->updateDeviceList();

                    unique_lock<mutex> lock(mutex_);
                    cv_.wait_for(lock, chrono::seconds(intervalSeconds),
                                 [this] { return !running_; });
                }
            });
        }

        void stop() {
            if (!running_) {
                return;
            }

            running_ = false;
            cv_.notify_all();

            if (thread_.joinable()) {
                thread_.join();
            }
        }

      private:
        QuestAdbManager* manager_;
        thread thread_;
        mutex mutex_;
        condition_variable cv_;
        atomic<bool> running_;
    };

    QuestAdbManager::QuestAdbManager() : QuestAdbManager("") {}

    QuestAdbManager::QuestAdbManager(const string& adbPath) {
        string actualAdbPath = adbPath.empty() ? findAdbPath() : adbPath;
        adbCommand_ = make_shared<AdbCommand>(actualAdbPath);
        monitoringThread_ = make_unique<MonitoringThread>(this);
    }

    QuestAdbManager::~QuestAdbManager() { stopDeviceMonitoring(); }

    Result<bool> QuestAdbManager::initialize() {
        auto result = adbCommand_->isAdbAvailable();
        if (!result.success) {
            return Result<bool>::Error("ADB not available: " + result.error);
        }

        initialized_ = result.value;
        return Result<bool>::Success(initialized_);
    }

    Result<vector<DeviceInfo>> QuestAdbManager::getConnectedDevices() {
        if (!initialized_) {
            return Result<vector<DeviceInfo>>::Error("Manager not initialized");
        }

        auto result = adbCommand_->getDevicesWithStatus();
        if (!result.success) {
            return Result<vector<DeviceInfo>>::Error(result.error);
        }

        vector<DeviceInfo> devices;
        for (const auto& deviceInfo : result.value) {
            // Update device info with detailed information
            auto device = getDevice(deviceInfo.deviceId);
            if (device.success) {
                auto detailedInfo = device.value->getDeviceInfo();
                if (detailedInfo.success) {
                    devices.push_back(detailedInfo.value);
                } else {
                    devices.push_back(deviceInfo);
                }
            } else {
                devices.push_back(deviceInfo);
            }
        }

        return Result<vector<DeviceInfo>>::Success(devices);
    }

    Result<shared_ptr<AdbDevice>> QuestAdbManager::getDevice(const string& deviceId) {
        if (!initialized_) {
            return Result<shared_ptr<AdbDevice>>::Error("Manager not initialized");
        }

        auto it = devices_.find(deviceId);
        if (it == devices_.end()) {
            auto device = make_shared<AdbDevice>(deviceId, adbCommand_);
            devices_[deviceId] = device;
            return Result<shared_ptr<AdbDevice>>::Success(device);
        }

        return Result<shared_ptr<AdbDevice>>::Success(it->second);
    }

    Result<bool> QuestAdbManager::refreshDeviceList() {
        return getConnectedDevices().success ? Result<bool>::Success(true)
                                             : Result<bool>::Error("Failed to refresh device list");
    }

    void QuestAdbManager::setDeviceStatusCallback(DeviceStatusCallback callback) {
        deviceStatusCallback_ = callback;
    }

    void QuestAdbManager::setDeviceListCallback(DeviceListCallback callback) {
        deviceListCallback_ = callback;
    }

    void QuestAdbManager::setMetricsProgressCallback(MetricsProgressCallback callback) {
        metricsProgressCallback_ = callback;
    }

    Result<bool> QuestAdbManager::startDeviceMonitoring(int intervalSeconds) {
        if (!initialized_) {
            return Result<bool>::Error("Manager not initialized");
        }

        if (monitoring_) {
            return Result<bool>::Success(true);
        }

        monitoring_ = true;
        monitoringThread_->start(intervalSeconds);

        return Result<bool>::Success(true);
    }

    void QuestAdbManager::stopDeviceMonitoring() {
        if (!monitoring_) {
            return;
        }

        monitoring_ = false;
        monitoringThread_->stop();
    }

    Result<bool> QuestAdbManager::rebootAndWaitAll() {
        auto devicesResult = getConnectedDevices();
        if (!devicesResult.success) {
            return Result<bool>::Error(devicesResult.error);
        }

        bool allSuccess = true;
        for (const auto& deviceInfo : devicesResult.value) {
            auto device = getDevice(deviceInfo.deviceId);
            if (device.success) {
                auto rebootResult = device.value->reboot();
                if (!rebootResult.success) {
                    allSuccess = false;
                    cerr << "Failed to reboot device " << deviceInfo.deviceId << ": "
                              << rebootResult.error << endl;
                } else {
                    auto waitResult =
                        device.value->waitForDevice(defaultConfig_.bootTimeoutSeconds);
                    if (!waitResult.success) {
                        allSuccess = false;
                        cerr << "Device " << deviceInfo.deviceId
                                  << " failed to come back online" << endl;
                    }
                }
            } else {
                allSuccess = false;
            }
        }

        return Result<bool>::Success(allSuccess);
    }

    Result<bool> QuestAdbManager::applyConfigurationAll(const HeadsetConfig& config) {
        auto devicesResult = getConnectedDevices();
        if (!devicesResult.success) {
            return Result<bool>::Error(devicesResult.error);
        }

        bool allSuccess = true;
        for (const auto& deviceInfo : devicesResult.value) {
            auto device = getDevice(deviceInfo.deviceId);
            if (device.success) {
                auto configResult = device.value->applyConfiguration(config);
                if (!configResult.success) {
                    allSuccess = false;
                    cerr << "Failed to apply configuration to device " << deviceInfo.deviceId
                              << ": " << configResult.error << endl;
                }
            } else {
                allSuccess = false;
            }
        }

        return Result<bool>::Success(allSuccess);
    }

    Result<map<string, bool>>
    QuestAdbManager::runCommandOnAll(const string& command) {
        auto devicesResult = getConnectedDevices();
        if (!devicesResult.success) {
            return Result<map<string, bool>>::Error(devicesResult.error);
        }

        map<string, bool> results;
        for (const auto& deviceInfo : devicesResult.value) {
            auto device = getDevice(deviceInfo.deviceId);
            if (device.success) {
                auto shellResult = device.value->shell(command);
                results[deviceInfo.deviceId] = shellResult.success;
            } else {
                results[deviceInfo.deviceId] = false;
            }
        }

        return Result<map<string, bool>>::Success(results);
    }

    Result<bool> QuestAdbManager::startMetricsRecordingAll(chrono::seconds duration) {
        auto devicesResult = getConnectedDevices();
        if (!devicesResult.success) {
            return Result<bool>::Error(devicesResult.error);
        }

        bool allSuccess = true;
        for (const auto& deviceInfo : devicesResult.value) {
            auto device = getDevice(deviceInfo.deviceId);
            if (device.success) {
                auto startResult = device.value->startMetricsRecording();
                if (startResult.success) {
                    MetricsSession session(deviceInfo.deviceId, duration);
                    session.isRecording = true;
                    activeSessions_[deviceInfo.deviceId] = session;
                } else {
                    allSuccess = false;
                }
            } else {
                allSuccess = false;
            }
        }

        return Result<bool>::Success(allSuccess);
    }

    Result<bool> QuestAdbManager::stopMetricsRecordingAll() {
        bool allSuccess = true;

        for (auto& [deviceId, session] : activeSessions_) {
            if (session.isRecording) {
                auto device = getDevice(deviceId);
                if (device.success) {
                    auto stopResult = device.value->stopMetricsRecording();
                    if (stopResult.success) {
                        session.isRecording = false;
                    } else {
                        allSuccess = false;
                    }
                } else {
                    allSuccess = false;
                }
            }
        }

        return Result<bool>::Success(allSuccess);
    }

    Result<map<string, string>>
    QuestAdbManager::pullMetricsAll(const string& localDirectory) {
        map<string, string> results;

        for (const auto& [deviceId, session] : activeSessions_) {
            auto device = getDevice(deviceId);
            if (device.success) {
                auto pullResult = device.value->pullLatestMetrics(localDirectory);
                if (pullResult.success) {
                    results[deviceId] = pullResult.value;
                } else {
                    results[deviceId] = "";
                }
            } else {
                results[deviceId] = "";
            }
        }

        return Result<map<string, string>>::Success(results);
    }

    void QuestAdbManager::setDefaultConfiguration(const HeadsetConfig& config) {
        defaultConfig_ = config;
    }

    const HeadsetConfig& QuestAdbManager::getDefaultConfiguration() const { return defaultConfig_; }

    string QuestAdbManager::findAdbPath() {
        AdbCommand tempCommand("");
        return tempCommand.getAdbPath();
    }

    Result<bool> QuestAdbManager::isAdbAvailable(const string& adbPath) {
        AdbCommand tempCommand(adbPath);
        return tempCommand.isAdbAvailable();
    }

    vector<string> QuestAdbManager::getPlatformToolsPaths() {
        AdbCommand tempCommand("");
        return tempCommand.getAdbSearchPaths();
    }

    string QuestAdbManager::getVersion() { return "1.0.0"; }

    string QuestAdbManager::getBuildInfo() { return "QuestAdbLib v1.0.0 - Built with CMake"; }

    void QuestAdbManager::updateDeviceList() {
        auto devicesResult = getConnectedDevices();
        if (devicesResult.success) {
            emitDeviceListUpdate(devicesResult.value);
        }
    }

    void QuestAdbManager::emitDeviceStatusChange(const string& deviceId,
                                                 const string& status) {
        if (deviceStatusCallback_) {
            deviceStatusCallback_(deviceId, status);
        }
    }

    void QuestAdbManager::emitDeviceListUpdate(const vector<DeviceInfo>& devices) {
        if (deviceListCallback_) {
            deviceListCallback_(devices);
        }
    }

    void QuestAdbManager::emitMetricsProgress(const string& deviceId, double progress) {
        if (metricsProgressCallback_) {
            metricsProgressCallback_(deviceId, progress);
        }
    }

} // namespace QuestAdbLib