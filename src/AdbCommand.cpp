#include "../include/QuestAdbLib/AdbCommand.h"
#include "Utils.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <regex>
#include <thread>

namespace QuestAdbLib {
    class AdbCommand::Impl {
      public:
        std::string adbPath;

        Impl(const std::string& path) : adbPath(path) {}
    };

    AdbCommand::AdbCommand(const std::string& adbPath)
        : adbPath_(adbPath.empty() ? findAdbPath() : adbPath),
          pImpl_(std::make_unique<Impl>(adbPath_)) {}

    AdbCommand::~AdbCommand() = default;

    std::string AdbCommand::findAdbPath() const {
        const std::string executable =
#ifdef _WIN32
            "adb.exe";
#else
            "adb";
#endif

        for (const auto& path : getAdbSearchPaths()) {
            std::string fullPath = Utils::joinPath(path, executable);
            if (Utils::fileExists(fullPath)) {
                std::cout << "Found ADB at: " << fullPath << std::endl;
                return fullPath;
            }
        }

        std::cout << "Using ADB from PATH" << std::endl;
        return executable;
    }

    std::vector<std::string> AdbCommand::getAdbSearchPaths() const {
        std::vector<std::string> paths;

        // Check for ADB next to the library DLL/executable first (highest priority)
        std::string exePath = Utils::getCurrentExecutablePath();
        if (!exePath.empty()) {
            std::string exeDir = Utils::getDirectoryFromPath(exePath);
            paths.push_back(exeDir);
        }

        // Check bundled ADB in platform-tools subdirectories
        if (!exePath.empty()) {
            std::string exeDir = Utils::getDirectoryFromPath(exePath);
            paths.push_back(Utils::joinPath(exeDir, "platform-tools"));

#ifdef _WIN32
            paths.push_back(Utils::joinPath(exeDir, "platform-tools/win32"));
#elif defined(__APPLE__)
            paths.push_back(Utils::joinPath(exeDir, "platform-tools/darwin"));
#else
            paths.push_back(Utils::joinPath(exeDir, "platform-tools/linux"));
#endif
        }

        // Add current working directory
        paths.push_back(Utils::joinPath(Utils::getCurrentWorkingDirectory(), "platform-tools"));

#ifdef _WIN32
        paths.push_back("C:\\adb");
        paths.push_back("C:\\Android\\platform-tools");
        paths.push_back("C:\\Program Files\\Android\\platform-tools");

        std::string localAppData = Utils::getEnvironmentVariable("LOCALAPPDATA");
        if (!localAppData.empty()) {
            paths.push_back(Utils::joinPath(localAppData, "Android\\Sdk\\platform-tools"));
        }

        std::string userProfile = Utils::getEnvironmentVariable("USERPROFILE");
        if (!userProfile.empty()) {
            paths.push_back(
                Utils::joinPath(userProfile, "AppData\\Local\\Android\\Sdk\\platform-tools"));
        }
#else
        paths.push_back("/usr/local/bin");
        paths.push_back("/opt/android-sdk/platform-tools");

        std::string home = Utils::getEnvironmentVariable("HOME");
        if (!home.empty()) {
            paths.push_back(Utils::joinPath(home, "Android/Sdk/platform-tools"));
        }
#endif

        return paths;
    }

    Result<std::string> AdbCommand::run(const std::string& command, const CommandOptions& options) {
        std::string quotedAdbPath = Utils::quoteStringIfNeeded(adbPath_);
        std::string fullCommand = quotedAdbPath + " " + command;

        auto result =
            Utils::executeCommand(fullCommand, options.timeoutSeconds, options.progressCallback);

        if (!result.success) {
            std::cerr << "ADB command failed: " << fullCommand << std::endl;
            std::cerr << "Error: " << result.error << std::endl;
            return Result<std::string>::Error("ADB command failed: " + result.error);
        }

        if (options.captureOutput) {
            // Filter out common ADB daemon messages from stderr
            if (!result.error.empty() && result.error.find("daemon") == std::string::npos &&
                result.error.find("Warning") == std::string::npos) {
                std::cerr << "ADB stderr: " << result.error << std::endl;
            }
            return Result<std::string>::Success(Utils::trim(result.output));
        }

        return Result<std::string>::Success("success");
    }

    Result<std::string> AdbCommand::runWithProgress(const std::string& command,
                                                    ProgressCallback progressCallback) {
        CommandOptions options;
        options.captureOutput = true;
        options.progressCallback = progressCallback;
        return run(command, options);
    }

    Result<bool> AdbCommand::isAdbAvailable() {
        auto result = run("version");
        return Result<bool>::Success(result.success);
    }

    Result<std::vector<std::string>> AdbCommand::getDevices() {
        auto result = run("devices");
        if (!result) {
            return Result<std::vector<std::string>>::Error(result.error);
        }

        std::vector<std::string> devices;
        auto lines = Utils::split(result.value, '\n');

        for (const auto& line : lines) {
            if (line.find("device") != std::string::npos &&
                line.find("List") == std::string::npos) {
                auto parts = Utils::split(line, '\t');
                if (!parts.empty() && !parts[0].empty()) {
                    devices.push_back(parts[0]);
                }
            }
        }

        return Result<std::vector<std::string>>::Success(devices);
    }

    Result<std::vector<DeviceInfo>> AdbCommand::getDevicesWithStatus() {
        auto result = run("devices");
        if (!result) {
            return Result<std::vector<DeviceInfo>>::Error(result.error);
        }

        std::vector<DeviceInfo> devices;
        auto lines = Utils::split(result.value, '\n');

        for (const auto& line : lines) {
            if ((line.find("device") != std::string::npos ||
                 line.find("unauthorized") != std::string::npos) &&
                line.find("List") == std::string::npos) {
                auto parts = Utils::split(line, '\t');
                if (parts.size() >= 2 && !parts[0].empty()) {
                    DeviceInfo deviceInfo(parts[0], parts[1]);
                    devices.push_back(deviceInfo);
                }
            }
        }

        return Result<std::vector<DeviceInfo>>::Success(devices);
    }

    Result<bool> AdbCommand::waitForDevice(const std::string& deviceId, int timeoutSeconds) {
        auto result = run("-s " + deviceId + " wait-for-device");
        if (!result) {
            return Result<bool>::Error(result.error);
        }

        // Wait for boot completion
        auto startTime = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - startTime <
               std::chrono::seconds(timeoutSeconds)) {
            auto bootResult = run("-s " + deviceId + " shell getprop sys.boot_completed");
            if (bootResult && Utils::trim(bootResult.value) == "1") {
                return Result<bool>::Success(true);
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        return Result<bool>::Success(false);
    }

    Result<bool> AdbCommand::reboot(const std::string& deviceId) {
        auto result = run("-s " + deviceId + " reboot");
        return Result<bool>::Success(result.success);
    }

    Result<std::string> AdbCommand::shell(const std::string& deviceId, const std::string& command,
                                          bool capture) {
        CommandOptions options;
        options.captureOutput = capture;

        auto result = run("-s " + deviceId + " shell " + command, options);
        if (!result) {
            return Result<std::string>::Error(result.error);
        }

        return Result<std::string>::Success(result.value);
    }

    Result<bool> AdbCommand::push(const std::string& deviceId, const std::string& localPath,
                                  const std::string& remotePath) {
        std::string quotedLocalPath = Utils::quoteStringIfNeeded(localPath);
        std::string quotedRemotePath = Utils::quoteStringIfNeeded(remotePath);

        auto result = run("-s " + deviceId + " push " + quotedLocalPath + " " + quotedRemotePath);
        return Result<bool>::Success(result.success);
    }

    Result<bool> AdbCommand::pull(const std::string& deviceId, const std::string& remotePath,
                                  const std::string& localPath) {
        std::string quotedRemotePath = Utils::quoteStringIfNeeded(remotePath);
        std::string quotedLocalPath = Utils::quoteStringIfNeeded(localPath);

        auto result = run("-s " + deviceId + " pull " + quotedRemotePath + " " + quotedLocalPath);
        return Result<bool>::Success(result.success);
    }

    Result<bool> AdbCommand::broadcast(const std::string& deviceId, const std::string& action,
                                       const std::string& component) {
        std::string command = "-s " + deviceId + " shell am broadcast -a " + action;
        if (!component.empty()) {
            command += " -n " + component;
        }

        auto result = run(command);
        return Result<bool>::Success(result.success);
    }

    Result<std::vector<std::string>> AdbCommand::getRunningProcesses(const std::string& deviceId) {
        auto result = shell(deviceId, "dumpsys activity processes", true);
        if (!result) {
            return Result<std::vector<std::string>>::Error(result.error);
        }

        std::vector<std::string> apps;
        auto lines = Utils::split(result.value, '\n');

        for (const auto& line : lines) {
            if (line.find("ProcessRecord{") != std::string::npos ||
                line.find("app=ProcessRecord{") != std::string::npos) {
                // Extract package name from ProcessRecord entries
                std::regex packageRegex(R"(([a-zA-Z0-9_.]+\.[a-zA-Z0-9_.]+))");
                std::smatch match;
                if (std::regex_search(line, match, packageRegex)) {
                    std::string packageName = match[1].str();
                    if (packageName.find('.') != std::string::npos) {
                        apps.push_back(packageName);
                    }
                }
            } else if (line.find("PERS") != std::string::npos &&
                       line.find(":") != std::string::npos) {
                std::regex packageRegex(R"(([a-zA-Z0-9_.]+\.[a-zA-Z0-9_.]+))");
                std::smatch match;
                if (std::regex_search(line, match, packageRegex)) {
                    std::string packageName = match[1].str();
                    if (packageName.find('.') != std::string::npos) {
                        apps.push_back(packageName);
                    }
                }
            }
        }

        // Remove duplicates
        std::sort(apps.begin(), apps.end());
        apps.erase(std::unique(apps.begin(), apps.end()), apps.end());

        return Result<std::vector<std::string>>::Success(apps);
    }
} // namespace QuestAdbLib
