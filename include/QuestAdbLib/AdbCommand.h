#pragma once

#include "Export.h"
#include "Types.h"
#include <string>
#include <vector>

using namespace std;

namespace QuestAdbLib {

    class QUESTADBLIB_API AdbCommand {
      public:
        AdbCommand(const string& adbPath);

        // Basic ADB operations
        Result<string> run(const string& command, const CommandOptions& options = {});
        Result<string> runWithProgress(const string& command,
                                            ProgressCallback progressCallback = nullptr);

        // Device management
        Result<bool> isAdbAvailable();
        Result<vector<string>> getDevices();
        Result<vector<DeviceInfo>> getDevicesWithStatus();
        Result<bool> waitForDevice(const string& deviceId, int timeoutSeconds = 60);

        // Device operations
        Result<bool> reboot(const string& deviceId);
        Result<string> shell(const string& deviceId, const string& command,
                                  bool capture = true);
        Result<bool> push(const string& deviceId, const string& localPath,
                          const string& remotePath);
        Result<bool> pull(const string& deviceId, const string& remotePath,
                          const string& localPath);
        Result<bool> broadcast(const string& deviceId, const string& action,
                               const string& component = "");

        // Process management
        Result<vector<string>> getRunningProcesses(const string& deviceId);

        // Getters
        const string& getAdbPath() const { return adbPath_; }
        vector<string> getAdbSearchPaths() const;

      private:
        string adbPath_;

        string findAdbPath() const;
    };

} // namespace QuestAdbLib