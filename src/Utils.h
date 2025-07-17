#pragma once

#include "../include/QuestAdbLib/Types.h"
#include <string>
#include <vector>

namespace QuestAdbLib {
    namespace Utils {

        struct ProcessResult {
            bool success = false;
            int exitCode = -1;
            std::string output;
            std::string error;
        };

        std::vector<std::string> split(const std::string& str, char delimiter);
        std::string trim(const std::string& str);
        bool fileExists(const std::string& path);
        std::string getCurrentExecutablePath();
        std::string getDirectoryFromPath(const std::string& path);
        std::string joinPath(const std::string& path1, const std::string& path2);
        std::string quoteStringIfNeeded(const std::string& str);
        ProcessResult executeCommand(const std::string& command, int timeoutSeconds = 30,
                                     ProgressCallback progressCallback = nullptr);
        std::string getEnvironmentVariable(const std::string& name);
        std::string getCurrentWorkingDirectory();

    } // namespace Utils
} // namespace QuestAdbLib