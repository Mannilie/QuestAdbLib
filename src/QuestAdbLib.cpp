#include "../include/QuestAdbLib/QuestAdbLib.h"
#include "Utils.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

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
            thread_ = std::thread([this, intervalSeconds]() {
                while (running_) {
                    manager_->updateDeviceList();

                    std::unique_lock<std::mutex> lock(mutex_);
                    cv_.wait_for(lock, std::chrono::seconds(intervalSeconds),
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
        std::thread thread_;
        std::mutex mutex_;
        std::condition_variable cv_;
        std::atomic<bool> running_;
    };

    QuestAdbManager::QuestAdbManager() : QuestAdbManager("") {}

    QuestAdbManager::QuestAdbManager(const std::string& adbPath) {
        std::string actualAdbPath = adbPath.empty() ? findAdbPath() : adbPath;
        adbCommand_ = std::make_shared<AdbCommand>(actualAdbPath);
        monitoringThread_ = std::make_unique<MonitoringThread>(this);
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

    Result<std::vector<DeviceInfo>> QuestAdbManager::getConnectedDevices() {
        if (!initialized_) {
            return Result<std::vector<DeviceInfo>>::Error("Manager not initialized");
        }

        auto result = adbCommand_->getDevicesWithStatus();
        if (!result.success) {
            return Result<std::vector<DeviceInfo>>::Error(result.error);
        }

        std::vector<DeviceInfo> devices;
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

        return Result<std::vector<DeviceInfo>>::Success(devices);
    }

    Result<std::shared_ptr<AdbDevice>> QuestAdbManager::getDevice(const std::string& deviceId) {
        if (!initialized_) {
            return Result<std::shared_ptr<AdbDevice>>::Error("Manager not initialized");
        }

        auto it = devices_.find(deviceId);
        if (it == devices_.end()) {
            auto device = std::make_shared<AdbDevice>(deviceId, adbCommand_);
            devices_[deviceId] = device;
            return Result<std::shared_ptr<AdbDevice>>::Success(device);
        }

        return Result<std::shared_ptr<AdbDevice>>::Success(it->second);
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
                    std::cerr << "Failed to reboot device " << deviceInfo.deviceId << ": "
                              << rebootResult.error << std::endl;
                } else {
                    auto waitResult =
                        device.value->waitForDevice(defaultConfig_.bootTimeoutSeconds);
                    if (!waitResult.success) {
                        allSuccess = false;
                        std::cerr << "Device " << deviceInfo.deviceId
                                  << " failed to come back online" << std::endl;
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
                    std::cerr << "Failed to apply configuration to device " << deviceInfo.deviceId
                              << ": " << configResult.error << std::endl;
                }
            } else {
                allSuccess = false;
            }
        }

        return Result<bool>::Success(allSuccess);
    }

    Result<std::map<std::string, bool>>
    QuestAdbManager::runCommandOnAll(const std::string& command) {
        auto devicesResult = getConnectedDevices();
        if (!devicesResult.success) {
            return Result<std::map<std::string, bool>>::Error(devicesResult.error);
        }

        std::map<std::string, bool> results;
        for (const auto& deviceInfo : devicesResult.value) {
            auto device = getDevice(deviceInfo.deviceId);
            if (device.success) {
                auto shellResult = device.value->shell(command);
                results[deviceInfo.deviceId] = shellResult.success;
            } else {
                results[deviceInfo.deviceId] = false;
            }
        }

        return Result<std::map<std::string, bool>>::Success(results);
    }

    Result<bool> QuestAdbManager::startMetricsRecordingAll(std::chrono::seconds duration) {
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

    Result<std::map<std::string, std::string>>
    QuestAdbManager::pullMetricsAll(const std::string& localDirectory) {
        std::map<std::string, std::string> results;

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

        return Result<std::map<std::string, std::string>>::Success(results);
    }

    void QuestAdbManager::setDefaultConfiguration(const HeadsetConfig& config) {
        defaultConfig_ = config;
    }

    const HeadsetConfig& QuestAdbManager::getDefaultConfiguration() const { return defaultConfig_; }

    std::string QuestAdbManager::findAdbPath() {
        AdbCommand tempCommand("");
        return tempCommand.getAdbPath();
    }

    Result<bool> QuestAdbManager::isAdbAvailable(const std::string& adbPath) {
        AdbCommand tempCommand(adbPath);
        return tempCommand.isAdbAvailable();
    }

    std::vector<std::string> QuestAdbManager::getPlatformToolsPaths() {
        AdbCommand tempCommand("");
        return tempCommand.getAdbSearchPaths();
    }

    std::string QuestAdbManager::getVersion() { return "1.0.0"; }

    std::string QuestAdbManager::getBuildInfo() { return "QuestAdbLib v1.0.0 - Built with CMake"; }

    void QuestAdbManager::updateDeviceList() {
        auto devicesResult = getConnectedDevices();
        if (devicesResult.success) {
            emitDeviceListUpdate(devicesResult.value);
        }
    }

    void QuestAdbManager::emitDeviceStatusChange(const std::string& deviceId,
                                                 const std::string& status) {
        if (deviceStatusCallback_) {
            deviceStatusCallback_(deviceId, status);
        }
    }

    void QuestAdbManager::emitDeviceListUpdate(const std::vector<DeviceInfo>& devices) {
        if (deviceListCallback_) {
            deviceListCallback_(devices);
        }
    }

    void QuestAdbManager::emitMetricsProgress(const std::string& deviceId, double progress) {
        if (metricsProgressCallback_) {
            metricsProgressCallback_(deviceId, progress);
        }
    }

} // namespace QuestAdbLib