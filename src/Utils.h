#pragma once

#include "../include/QuestAdbLib/Types.h"
#include <string>
#include <vector>

using namespace std;

namespace QuestAdbLib {
    namespace Utils {

        struct ProcessResult {
            bool success = false;
            int exitCode = -1;
            string output;
            string error;
        };

        vector<string> split(const string& str, char delimiter);
        string trim(const string& str);
        bool fileExists(const string& path);
        string getCurrentExecutablePath();
        string getDirectoryFromPath(const string& path);
        string joinPath(const string& path1, const string& path2);
        string quoteStringIfNeeded(const string& str);
        ProcessResult executeCommand(const string& command, int timeoutSeconds = 30,
                                     ProgressCallback progressCallback = nullptr);
        string getEnvironmentVariable(const string& name);
        string getCurrentWorkingDirectory();

    } // namespace Utils
} // namespace QuestAdbLib