#include "Utils.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace QuestAdbLib {
    namespace Utils {

        std::vector<std::string> split(const std::string& str, char delimiter) {
            std::vector<std::string> tokens;
            std::stringstream ss(str);
            std::string token;

            while (std::getline(ss, token, delimiter)) {
                if (!token.empty()) {
                    tokens.push_back(token);
                }
            }

            return tokens;
        }

        std::string trim(const std::string& str) {
            auto start = str.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) {
                return "";
            }

            auto end = str.find_last_not_of(" \t\r\n");
            return str.substr(start, end - start + 1);
        }

        bool fileExists(const std::string& path) { return std::filesystem::exists(path); }

        std::string getCurrentExecutablePath() {
#ifdef _WIN32
            char buffer[MAX_PATH];
            GetModuleFileNameA(NULL, buffer, MAX_PATH);
            return std::string(buffer);
#else
            char buffer[1024];
            ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
            if (count == -1) {
                return "";
            }
            buffer[count] = '\0';
            return std::string(buffer);
#endif
        }

        std::string getDirectoryFromPath(const std::string& path) {
            return std::filesystem::path(path).parent_path().string();
        }

        std::string joinPath(const std::string& path1, const std::string& path2) {
            return (std::filesystem::path(path1) / path2).string();
        }

        std::string quoteStringIfNeeded(const std::string& str) {
            if (str.find(' ') != std::string::npos) {
                return "\"" + str + "\"";
            }
            return str;
        }

        ProcessResult executeCommand(const std::string& command, int timeoutSeconds,
                                     ProgressCallback progressCallback) {
            ProcessResult result;
            result.success = false;
            result.exitCode = -1;

#ifdef _WIN32
            SECURITY_ATTRIBUTES saAttr;
            saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
            saAttr.bInheritHandle = TRUE;
            saAttr.lpSecurityDescriptor = NULL;

            HANDLE hChildStd_OUT_Rd, hChildStd_OUT_Wr;
            HANDLE hChildStd_ERR_Rd, hChildStd_ERR_Wr;

            if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0) ||
                !CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0)) {
                result.error = "Failed to create pipes";
                return result;
            }

            SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);
            SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0);

            PROCESS_INFORMATION piProcInfo;
            STARTUPINFOA siStartInfo;

            ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
            ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));

            siStartInfo.cb = sizeof(STARTUPINFOA);
            siStartInfo.hStdError = hChildStd_ERR_Wr;
            siStartInfo.hStdOutput = hChildStd_OUT_Wr;
            siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

            std::string cmdLine = "cmd.exe /c " + command;

            BOOL bSuccess = CreateProcessA(NULL, const_cast<char*>(cmdLine.c_str()), NULL, NULL,
                                           TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);

            if (!bSuccess) {
                result.error = "Failed to create process";
                CloseHandle(hChildStd_OUT_Rd);
                CloseHandle(hChildStd_OUT_Wr);
                CloseHandle(hChildStd_ERR_Rd);
                CloseHandle(hChildStd_ERR_Wr);
                return result;
            }

            CloseHandle(hChildStd_OUT_Wr);
            CloseHandle(hChildStd_ERR_Wr);

            // Read output
            DWORD dwRead;
            char buffer[4096];

            while (ReadFile(hChildStd_OUT_Rd, buffer, sizeof(buffer) - 1, &dwRead, NULL) &&
                   dwRead > 0) {
                buffer[dwRead] = '\0';
                result.output += buffer;
                if (progressCallback) {
                    progressCallback(buffer);
                }
            }

            while (ReadFile(hChildStd_ERR_Rd, buffer, sizeof(buffer) - 1, &dwRead, NULL) &&
                   dwRead > 0) {
                buffer[dwRead] = '\0';
                result.error += buffer;
                if (progressCallback) {
                    progressCallback(buffer);
                }
            }

            DWORD waitResult = WaitForSingleObject(piProcInfo.hProcess, timeoutSeconds * 1000);

            if (waitResult == WAIT_TIMEOUT) {
                TerminateProcess(piProcInfo.hProcess, 1);
                result.error = "Command timed out";
            } else if (waitResult == WAIT_OBJECT_0) {
                DWORD exitCode;
                if (GetExitCodeProcess(piProcInfo.hProcess, &exitCode)) {
                    result.exitCode = static_cast<int>(exitCode);
                    result.success = (exitCode == 0);
                }
            }

            CloseHandle(piProcInfo.hProcess);
            CloseHandle(piProcInfo.hThread);
            CloseHandle(hChildStd_OUT_Rd);
            CloseHandle(hChildStd_ERR_Rd);

#else
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                result.error = "Failed to create pipe";
                return result;
            }

            pid_t pid = fork();
            if (pid == -1) {
                result.error = "Failed to fork process";
                close(pipefd[0]);
                close(pipefd[1]);
                return result;
            } else if (pid == 0) {
                // Child process
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                dup2(pipefd[1], STDERR_FILENO);
                close(pipefd[1]);

                execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
                _exit(1);
            } else {
                // Parent process
                close(pipefd[1]);

                char buffer[4096];
                ssize_t bytesRead;

                while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                    buffer[bytesRead] = '\0';
                    result.output += buffer;
                    if (progressCallback) {
                        progressCallback(buffer);
                    }
                }

                close(pipefd[0]);

                int status;
                if (waitpid(pid, &status, 0) == -1) {
                    result.error = "Failed to wait for child process";
                } else {
                    if (WIFEXITED(status)) {
                        result.exitCode = WEXITSTATUS(status);
                        result.success = (result.exitCode == 0);
                    } else if (WIFSIGNALED(status)) {
                        result.error = "Process was terminated by signal";
                        result.exitCode = -1;
                    }
                }
            }
#endif

            return result;
        }

        std::string getEnvironmentVariable(const std::string& name) {
            const char* value = std::getenv(name.c_str());
            return value ? std::string(value) : std::string();
        }

        std::string getCurrentWorkingDirectory() {
            return std::filesystem::current_path().string();
        }

    } // namespace Utils
} // namespace QuestAdbLib