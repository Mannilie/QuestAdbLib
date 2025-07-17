#pragma once

#include "Export.h"
#include "Types.h"
#include <string>
#include <vector>
#include <memory>

namespace QuestAdbLib {

    class QUESTADBLIB_API AdbCommand {
    public:
        AdbCommand(const std::string& adbPath);
        ~AdbCommand();

        // Basic ADB operations
        Result<std::string> run(const std::string& command, const CommandOptions& options = {});
        Result<std::string> runWithProgress(const std::string& command, ProgressCallback progressCallback = nullptr);
        
        // Device management
        Result<bool> isAdbAvailable();
        Result<std::vector<std::string>> getDevices();
        Result<std::vector<DeviceInfo>> getDevicesWithStatus();
        Result<bool> waitForDevice(const std::string& deviceId, int timeoutSeconds = 60);
        
        // Device operations
        Result<bool> reboot(const std::string& deviceId);
        Result<std::string> shell(const std::string& deviceId, const std::string& command, bool capture = true);
        Result<bool> push(const std::string& deviceId, const std::string& localPath, const std::string& remotePath);
        Result<bool> pull(const std::string& deviceId, const std::string& remotePath, const std::string& localPath);
        Result<bool> broadcast(const std::string& deviceId, const std::string& action, const std::string& component = "");
        
        // Process management
        Result<std::vector<std::string>> getRunningProcesses(const std::string& deviceId);
        
        // Getters
        const std::string& getAdbPath() const { return adbPath_; }
        std::vector<std::string> getAdbSearchPaths() const;
        
    private:
        std::string adbPath_;
        class Impl;
        std::unique_ptr<Impl> pImpl_;
        
        std::string findAdbPath() const;
    };

} // namespace QuestAdbLib