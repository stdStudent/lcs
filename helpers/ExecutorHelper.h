//
// Created by Labour on 07.01.2024.
//

#ifndef EXECUTORHELPER_H
#define EXECUTORHELPER_H

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <string>
#include <vector>

#include "ErrorHelper.h"
#include "OutputCapturerHelper.h"

class ExecutorHelper {
    // Structure to hold command arguments
    typedef struct {
        char *argv[100];
    } Command;

    // Structure for linked list nodes
    typedef struct Node {
        Command cmd;
        Node *next;
    } Node;

    // Function to parse command
    static Node *parse_command(char *input) {
        const auto node = static_cast<Node *>(malloc(sizeof(Node)));
        char *token = strtok(input, " ");
        int i = 0;
        while (token != nullptr) {
            node->cmd.argv[i] = token;
            i++;
            token = strtok(nullptr, " ");
        }
        node->cmd.argv[i] = nullptr;
        node->next = nullptr;
        return node;
    }

public:
    static std::string execute(const std::vector<std::string>& commands) {
        pid_t pid;

        // Create linked list of commands
        Node *head = nullptr;
        Node *tail = nullptr;
        for (const std::string& command: commands)  {
            Node *node = parse_command(strdup(command.c_str()));
            if (head == nullptr) {
                head = node;
                tail = node;
            } else {
                if (tail != nullptr) {
                    tail->next = node;
                    tail = node;
                }
            }
        }

        OutputCapturerHelper och;
        if (och.start() == false) {
            return EX_MODULE "Coudln't start " OUTPUT_CAPTURER_HELPER;
        }

        // Iterate through linked list
        const Node* current = head;
        while (current != nullptr) {
            int pipefd[2];
            // Create pipe
            if (pipe(pipefd) == -1) {
                return ErrorHelper::perror(EX_MODULE "pipe");
            }

            // Fork child process
            if ((pid = fork()) == -1) {
                return ErrorHelper::perror(EX_MODULE "fork");
            } else if (pid == 0) { // Child process
                if (current->next != nullptr) {
                    close(pipefd[0]); // Close read end of pipe
                    dup2(pipefd[1], STDOUT_FILENO); // Duplicate write end of pipe to stdout
                    close(pipefd[1]); // Close write end of pipe
                }
                execvp(current->cmd.argv[0], current->cmd.argv); // Execute command
                return ErrorHelper::perror(EX_MODULE "execvp"); // execvp returns only on error
            } else { // Parent process
                if (current->next != nullptr) {
                    close(pipefd[1]); // Close write end of pipe
                    dup2(pipefd[0], STDIN_FILENO); // Duplicate read end of pipe to stdin
                    close(pipefd[0]); // Close read end of pipe
                }

                wait(nullptr); // Wait for child process to finish
            }
            current = current->next;
        }

        const auto& result = och.stop();

        // Free linked list
        while (head != nullptr) {
            Node *temp = head;
            head = head->next;
            free(temp);
        }

        return result;
    }
};

#endif //EXECUTORHELPER_H
