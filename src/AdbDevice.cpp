#include "../include/QuestAdbLib/AdbDevice.h"
#include "Utils.h"
#include <regex>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace QuestAdbLib {

AdbDevice::AdbDevice(const std::string& deviceId, std::shared_ptr<AdbCommand> adbCommand)
    : deviceId_(deviceId), adbCommand_(adbCommand) {
}

AdbDevice::~AdbDevice() = default;

Result<DeviceInfo> AdbDevice::getDeviceInfo() {
    DeviceInfo info(deviceId_, "connected");
    
    // Get model
    auto modelResult = getModel();
    if (modelResult) {
        info.model = modelResult.value;
    }
    
    // Get battery level
    auto batteryResult = getBatteryLevel();
    if (batteryResult) {
        info.batteryLevel = batteryResult.value;
    }
    
    // Get running apps
    auto appsResult = getRunningApps();
    if (appsResult) {
        info.runningApps = appsResult.value;
    }
    
    info.lastUpdated = std::chrono::system_clock::now();
    
    return Result<DeviceInfo>::Success(info);
}

Result<std::string> AdbDevice::getModel() {
    return adbCommand_->shell(deviceId_, "getprop ro.product.model", true);
}

Result<int> AdbDevice::getBatteryLevel() {
    auto result = adbCommand_->shell(deviceId_, "dumpsys battery", true);
    if (!result) {
        return Result<int>::Error(result.error);
    }
    
    std::regex batteryRegex(R"(level: (\d+))");
    std::smatch match;
    if (std::regex_search(result.value, match, batteryRegex)) {
        return Result<int>::Success(std::stoi(match[1].str()));
    }
    
    return Result<int>::Error("Could not parse battery level");
}

Result<std::vector<std::string>> AdbDevice::getRunningApps() {
    return adbCommand_->getRunningProcesses(deviceId_);
}

Result<bool> AdbDevice::reboot() {
    return adbCommand_->reboot(deviceId_);
}

Result<bool> AdbDevice::waitForDevice(int timeoutSeconds) {
    return adbCommand_->waitForDevice(deviceId_, timeoutSeconds);
}

Result<bool> AdbDevice::applyConfiguration(const HeadsetConfig& config) {
    bool allSuccess = true;
    
    // Apply CPU level
    if (config.cpuLevel >= 0 && config.cpuLevel <= 4) {
        auto result = setCpuLevel(config.cpuLevel);
        if (!result.success) {
            allSuccess = false;
        }
    }
    
    // Apply GPU level
    if (config.gpuLevel >= 0 && config.gpuLevel <= 4) {
        auto result = setGpuLevel(config.gpuLevel);
        if (!result.success) {
            allSuccess = false;
        }
    }
    
    // Disable proximity sensor
    if (config.disableProximity) {
        auto result = disableProximity();
        if (!result.success) {
            allSuccess = false;
        }
    }
    
    // Disable Guardian
    if (config.disableGuardian) {
        auto result = disableGuardian();
        if (!result.success) {
            allSuccess = false;
        }
    }
    
    // Disable metrics overlay and CSV recording
    disableMetricsOverlay();
    disableCsvMetrics();
    clearMetricsFiles();
    
    return Result<bool>::Success(allSuccess);
}

Result<std::string> AdbDevice::shell(const std::string& command, bool capture) {
    return adbCommand_->shell(deviceId_, command, capture);
}

Result<bool> AdbDevice::setProperty(const std::string& property, const std::string& value) {
    auto result = shell("setprop " + property + " " + value);
    return Result<bool>::Success(result.success);
}

Result<std::string> AdbDevice::getProperty(const std::string& property) {
    return shell("getprop " + property, true);
}

Result<bool> AdbDevice::pushFile(const std::string& localPath, const std::string& remotePath) {
    return adbCommand_->push(deviceId_, localPath, remotePath);
}

Result<bool> AdbDevice::pullFile(const std::string& remotePath, const std::string& localPath) {
    return adbCommand_->pull(deviceId_, remotePath, localPath);
}

Result<bool> AdbDevice::removeFile(const std::string& remotePath) {
    std::string quotedPath = Utils::quoteStringIfNeeded(remotePath);
    auto result = shell("rm -f " + quotedPath);
    return Result<bool>::Success(result.success);
}

Result<bool> AdbDevice::fileExists(const std::string& remotePath) {
    std::string quotedPath = Utils::quoteStringIfNeeded(remotePath);
    auto result = shell("ls " + quotedPath, true);
    return Result<bool>::Success(result.success && result.value.find("No such file") == std::string::npos);
}

Result<bool> AdbDevice::sendBroadcast(const std::string& action, const std::string& component) {
    return adbCommand_->broadcast(deviceId_, action, component);
}

Result<bool> AdbDevice::startMetricsRecording() {
    // Clear old metrics first
    clearMetricsFiles();
    
    // Enable overlay
    auto overlayResult = sendBroadcast("com.oculus.ovrmonitormetricsservice.ENABLE_OVERLAY", METRICS_SERVICE_COMPONENT);
    if (!overlayResult.success) {
        return Result<bool>::Error("Failed to enable metrics overlay");
    }
    
    // Enable CSV recording
    auto csvResult = enableCsvMetrics();
    if (!csvResult.success) {
        return Result<bool>::Error("Failed to enable CSV metrics");
    }
    
    return Result<bool>::Success(true);
}

Result<bool> AdbDevice::stopMetricsRecording() {
    // Disable CSV recording
    auto csvResult = disableCsvMetrics();
    
    // Disable overlay
    auto overlayResult = disableMetricsOverlay();
    
    return Result<bool>::Success(csvResult.success && overlayResult.success);
}

Result<bool> AdbDevice::clearMetricsFiles() {
    // First disable CSV recording
    disableCsvMetrics();
    
    // Remove old CSV files
    std::string command = "rm -f \"" + std::string(DEVICE_METRICS_PATH) + "/*.csv\"";
    auto result = shell(command);
    
    return Result<bool>::Success(result.success);
}

Result<std::vector<std::string>> AdbDevice::getMetricsFiles() {
    std::string command = "ls \"" + std::string(DEVICE_METRICS_PATH) + "\"";
    auto result = shell(command, true);
    
    if (!result.success) {
        return Result<std::vector<std::string>>::Error(result.error);
    }
    
    std::vector<std::string> csvFiles;
    auto lines = Utils::split(result.value, '\n');
    
    for (const auto& line : lines) {
        std::string filename = Utils::trim(line);
        if (!filename.empty() && filename.find(".csv") != std::string::npos &&
            filename.find("No such file") == std::string::npos) {
            csvFiles.push_back(filename);
        }
    }
    
    return Result<std::vector<std::string>>::Success(csvFiles);
}

Result<std::string> AdbDevice::pullLatestMetrics(const std::string& localDirectory) {
    auto filesResult = getMetricsFiles();
    if (!filesResult.success) {
        return Result<std::string>::Error(filesResult.error);
    }
    
    if (filesResult.value.empty()) {
        return Result<std::string>::Error("No metrics files found on device");
    }
    
    // Get the latest file (assuming they're sorted alphabetically/chronologically)
    std::vector<std::string> files = filesResult.value;
    std::sort(files.begin(), files.end());
    std::string latestFile = files.back();
    
    // Generate local filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H-%M-%S");
    
    std::string localFilename = "metrics_" + deviceId_ + "_" + ss.str() + ".csv";
    std::string localPath = Utils::joinPath(localDirectory, localFilename);
    
    // Create directory if it doesn't exist
    std::filesystem::create_directories(localDirectory);
    
    // Pull the file
    std::string remotePath = std::string(DEVICE_METRICS_PATH) + "/" + latestFile;
    auto pullResult = pullFile(remotePath, localPath);
    
    if (!pullResult.success) {
        return Result<std::string>::Error("Failed to pull metrics file");
    }
    
    // Verify file was pulled successfully
    if (!Utils::fileExists(localPath)) {
        return Result<std::string>::Error("Failed to pull metrics file from device");
    }
    
    return Result<std::string>::Success(localPath);
}

Result<bool> AdbDevice::setCpuLevel(int level) {
    return setProperty("debug.oculus.cpuLevel", std::to_string(level));
}

Result<bool> AdbDevice::setGpuLevel(int level) {
    return setProperty("debug.oculus.gpuLevel", std::to_string(level));
}

Result<bool> AdbDevice::disableProximity() {
    return sendBroadcast("com.oculus.vrpowermanager.prox_close");
}

Result<bool> AdbDevice::disableGuardian() {
    return setProperty("debug.oculus.guardian_pause", "1");
}

Result<bool> AdbDevice::enableMetricsOverlay() {
    return sendBroadcast("com.oculus.ovrmonitormetricsservice.ENABLE_OVERLAY", METRICS_SERVICE_COMPONENT);
}

Result<bool> AdbDevice::disableMetricsOverlay() {
    return sendBroadcast("com.oculus.ovrmonitormetricsservice.DISABLE_OVERLAY", METRICS_SERVICE_COMPONENT);
}

Result<bool> AdbDevice::enableCsvMetrics() {
    return sendBroadcast("com.oculus.ovrmonitormetricsservice.ENABLE_CSV", METRICS_SERVICE_COMPONENT);
}

Result<bool> AdbDevice::disableCsvMetrics() {
    return sendBroadcast("com.oculus.ovrmonitormetricsservice.DISABLE_CSV", METRICS_SERVICE_COMPONENT);
}

Result<bool> AdbDevice::isAppRunning(const std::string& packageName) {
    auto result = getRunningApps();
    if (!result.success) {
        return Result<bool>::Error(result.error);
    }
    
    auto it = std::find(result.value.begin(), result.value.end(), packageName);
    return Result<bool>::Success(it != result.value.end());
}

Result<bool> AdbDevice::hasMetricsTriggerApps(const std::vector<std::string>& triggerApps) {
    auto result = getRunningApps();
    if (!result.success) {
        return Result<bool>::Error(result.error);
    }
    
    for (const auto& app : result.value) {
        if (std::find(triggerApps.begin(), triggerApps.end(), app) != triggerApps.end()) {
            return Result<bool>::Success(true);
        }
    }
    
    return Result<bool>::Success(false);
}

} // namespace QuestAdbLib