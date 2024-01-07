//
// Created by Labour on 07.01.2024.
//

#ifndef OUTPUTCAPTURERHELPER_H
#define OUTPUTCAPTURERHELPER_H

#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string>

#include "Defines.h"
#include "ErrorHelper.h"

class OutputCapturerHelper {
    std::string combinedOutput;
    int saved_stdout{}, saved_stderr{};
    int _pipefd[2][2]{};

public:
    bool start() {
        saved_stdout = dup(STDOUT_FILENO);
        saved_stderr = dup(STDERR_FILENO);

        if (pipe(_pipefd[0]) == -1 || pipe(_pipefd[1]) == -1) {
            perror(OUTPUT_CAPTURER_HELPER "pipe");
            return false;
        }

        if (dup2(_pipefd[0][1], STDOUT_FILENO) == -1 || dup2(_pipefd[1][1], STDERR_FILENO) == -1) {
            perror("dup2");
            return false;
        }

        // Set the pipes to non-blocking mode
        fcntl(_pipefd[0][0], F_SETFL, O_NONBLOCK);
        fcntl(_pipefd[1][0], F_SETFL, O_NONBLOCK);

        return true;
    }

    std::string stop(const bool clear = true) {
        std::string capturedStdout, capturedStderr;
        char buffer[1024];
        ssize_t count;

        while ((count = read(_pipefd[0][0], buffer, sizeof(buffer)-1)) > 0) {
            buffer[count] = '\0';
            capturedStdout += buffer;
        }
        if (count == -1 && errno != EAGAIN) {
            return ErrorHelper::perror("read");
        }

        while ((count = read(_pipefd[1][0], buffer, sizeof(buffer)-1)) > 0) {
            buffer[count] = '\0';
            capturedStderr += buffer;
        }

        if (count == -1 && errno != EAGAIN) {
            return ErrorHelper::perror(OUTPUT_CAPTURER_HELPER "read");
        }

        // Restore the original stdout and stderr file descriptors
        if (dup2(saved_stdout, STDOUT_FILENO) == -1 || dup2(saved_stderr, STDERR_FILENO) == -1) {
            return ErrorHelper::perror("dup2");
        }

        if (!capturedStdout.empty()) {
            combinedOutput += capturedStdout;
        }

        if (!capturedStderr.empty()) {
            if (!combinedOutput.empty()) {
                combinedOutput += "\nExecution returned errors (to stderr):\n";
            }
            combinedOutput += capturedStderr;
        }

        if (clear == true) {
            auto temp = combinedOutput;
            combinedOutput.clear();
            return temp;
        } else {
            return combinedOutput;
        }
    }
};

#endif //OUTPUTCAPTURERHELPER_H
