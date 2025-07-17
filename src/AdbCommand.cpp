#include "../include/QuestAdbLib/AdbCommand.h"
#include "Utils.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <regex>
#include <thread>

using namespace std;

namespace QuestAdbLib {
    AdbCommand::AdbCommand(const string& adbPath)
        : adbPath_(adbPath.empty() ? findAdbPath() : adbPath) {}

    string AdbCommand::findAdbPath() const {
        const string executable =
#ifdef _WIN32
            "adb.exe";
#else
            "adb";
#endif

        for (const auto& path : getAdbSearchPaths()) {
            string fullPath = Utils::joinPath(path, executable);
            if (Utils::fileExists(fullPath)) {
                cout << "Found ADB at: " << fullPath << endl;
                return fullPath;
            }
        }

        cout << "Using ADB from PATH" << endl;
        return executable;
    }

    vector<string> AdbCommand::getAdbSearchPaths() const {
        vector<string> paths;

        // Check for ADB next to the library DLL/executable first (highest priority)
        string exePath = Utils::getCurrentExecutablePath();
        if (!exePath.empty()) {
            string exeDir = Utils::getDirectoryFromPath(exePath);
            paths.push_back(exeDir);
        }

        // Check bundled ADB in platform-tools subdirectories
        if (!exePath.empty()) {
            string exeDir = Utils::getDirectoryFromPath(exePath);
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

        string localAppData = Utils::getEnvironmentVariable("LOCALAPPDATA");
        if (!localAppData.empty()) {
            paths.push_back(Utils::joinPath(localAppData, "Android\\Sdk\\platform-tools"));
        }

        string userProfile = Utils::getEnvironmentVariable("USERPROFILE");
        if (!userProfile.empty()) {
            paths.push_back(
                Utils::joinPath(userProfile, "AppData\\Local\\Android\\Sdk\\platform-tools"));
        }
#else
        paths.push_back("/usr/local/bin");
        paths.push_back("/opt/android-sdk/platform-tools");

        string home = Utils::getEnvironmentVariable("HOME");
        if (!home.empty()) {
            paths.push_back(Utils::joinPath(home, "Android/Sdk/platform-tools"));
        }
#endif

        return paths;
    }

    Result<string> AdbCommand::run(const string& command, const CommandOptions& options) {
        string quotedAdbPath = Utils::quoteStringIfNeeded(adbPath_);
        string fullCommand = quotedAdbPath + " " + command;

        auto result =
            Utils::executeCommand(fullCommand, options.timeoutSeconds, options.progressCallback);

        if (!result.success) {
            cerr << "ADB command failed: " << fullCommand << endl;
            cerr << "Error: " << result.error << endl;
            return Result<string>::Error("ADB command failed: " + result.error);
        }

        if (options.captureOutput) {
            // Filter out common ADB daemon messages from stderr
            if (!result.error.empty() && result.error.find("daemon") == string::npos &&
                result.error.find("Warning") == string::npos) {
                cerr << "ADB stderr: " << result.error << endl;
            }
            return Result<string>::Success(Utils::trim(result.output));
        }

        return Result<string>::Success("success");
    }

    Result<string> AdbCommand::runWithProgress(const string& command,
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

    Result<vector<string>> AdbCommand::getDevices() {
        auto result = run("devices");
        if (!result) {
            return Result<vector<string>>::Error(result.error);
        }

        vector<string> devices;
        auto lines = Utils::split(result.value, '\n');

        for (const auto& line : lines) {
            if (line.find("device") != string::npos &&
                line.find("List") == string::npos) {
                auto parts = Utils::split(line, '\t');
                if (!parts.empty() && !parts[0].empty()) {
                    devices.push_back(parts[0]);
                }
            }
        }

        return Result<vector<string>>::Success(devices);
    }

    Result<vector<DeviceInfo>> AdbCommand::getDevicesWithStatus() {
        auto result = run("devices");
        if (!result) {
            return Result<vector<DeviceInfo>>::Error(result.error);
        }

        vector<DeviceInfo> devices;
        auto lines = Utils::split(result.value, '\n');

        for (const auto& line : lines) {
            if ((line.find("device") != string::npos ||
                 line.find("unauthorized") != string::npos) &&
                line.find("List") == string::npos) {
                auto parts = Utils::split(line, '\t');
                if (parts.size() >= 2 && !parts[0].empty()) {
                    DeviceInfo deviceInfo(parts[0], parts[1]);
                    devices.push_back(deviceInfo);
                }
            }
        }

        return Result<vector<DeviceInfo>>::Success(devices);
    }

    Result<bool> AdbCommand::waitForDevice(const string& deviceId, int timeoutSeconds) {
        auto result = run("-s " + deviceId + " wait-for-device");
        if (!result) {
            return Result<bool>::Error(result.error);
        }

        // Wait for boot completion
        auto startTime = chrono::steady_clock::now();
        while (chrono::steady_clock::now() - startTime <
               chrono::seconds(timeoutSeconds)) {
            auto bootResult = run("-s " + deviceId + " shell getprop sys.boot_completed");
            if (bootResult && Utils::trim(bootResult.value) == "1") {
                return Result<bool>::Success(true);
            }

            this_thread::sleep_for(chrono::seconds(1));
        }

        return Result<bool>::Success(false);
    }

    Result<bool> AdbCommand::reboot(const string& deviceId) {
        auto result = run("-s " + deviceId + " reboot");
        return Result<bool>::Success(result.success);
    }

    Result<string> AdbCommand::shell(const string& deviceId, const string& command,
                                          bool capture) {
        CommandOptions options;
        options.captureOutput = capture;

        auto result = run("-s " + deviceId + " shell " + command, options);
        if (!result) {
            return Result<string>::Error(result.error);
        }

        return Result<string>::Success(result.value);
    }

    Result<bool> AdbCommand::push(const string& deviceId, const string& localPath,
                                  const string& remotePath) {
        string quotedLocalPath = Utils::quoteStringIfNeeded(localPath);
        string quotedRemotePath = Utils::quoteStringIfNeeded(remotePath);

        auto result = run("-s " + deviceId + " push " + quotedLocalPath + " " + quotedRemotePath);
        return Result<bool>::Success(result.success);
    }

    Result<bool> AdbCommand::pull(const string& deviceId, const string& remotePath,
                                  const string& localPath) {
        string quotedRemotePath = Utils::quoteStringIfNeeded(remotePath);
        string quotedLocalPath = Utils::quoteStringIfNeeded(localPath);

        auto result = run("-s " + deviceId + " pull " + quotedRemotePath + " " + quotedLocalPath);
        return Result<bool>::Success(result.success);
    }

    Result<bool> AdbCommand::broadcast(const string& deviceId, const string& action,
                                       const string& component) {
        string command = "-s " + deviceId + " shell am broadcast -a " + action;
        if (!component.empty()) {
            command += " -n " + component;
        }

        auto result = run(command);
        return Result<bool>::Success(result.success);
    }

    Result<vector<string>> AdbCommand::getRunningProcesses(const string& deviceId) {
        auto result = shell(deviceId, "dumpsys activity processes", true);
        if (!result) {
            return Result<vector<string>>::Error(result.error);
        }

        vector<string> apps;
        auto lines = Utils::split(result.value, '\n');

        for (const auto& line : lines) {
            if (line.find("ProcessRecord{") != string::npos ||
                line.find("app=ProcessRecord{") != string::npos) {
                // Extract package name from ProcessRecord entries
                regex packageRegex(R"(([a-zA-Z0-9_.]+\.[a-zA-Z0-9_.]+))");
                smatch match;
                if (regex_search(line, match, packageRegex)) {
                    string packageName = match[1].str();
                    if (packageName.find('.') != string::npos) {
                        apps.push_back(packageName);
                    }
                }
            } else if (line.find("PERS") != string::npos &&
                       line.find(":") != string::npos) {
                regex packageRegex(R"(([a-zA-Z0-9_.]+\.[a-zA-Z0-9_.]+))");
                smatch match;
                if (regex_search(line, match, packageRegex)) {
                    string packageName = match[1].str();
                    if (packageName.find('.') != string::npos) {
                        apps.push_back(packageName);
                    }
                }
            }
        }

        // Remove duplicates
        sort(apps.begin(), apps.end());
        apps.erase(unique(apps.begin(), apps.end()), apps.end());

        return Result<vector<string>>::Success(apps);
    }
} // namespace QuestAdbLib
