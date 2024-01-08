//
// Created by Labour on 05.01.2024.
//

#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <unordered_map>
#include <sys/socket.h> // for socket

#include "Defines.h"
#include "ExecutorHelper.h"
#include "ListFilesHelper.h"
#include "ProcessListHelper.h"

class CommandHandler {
    static inline const auto errorWrongCommand = "! Wrong command received: ";
    static inline const char* endOfMessage = END_OF_MESSAGE;

    const inline static std::unordered_map<std::string,int> commandToCase{
        {"default", DEFAULT_CASE},
        {"ps", PS},
        {"ls", LS},
        {"ex", EX}
    };

    static ssize_t sendall(const int s, const char *buf, const ssize_t len) {
        ssize_t total = 0;       // how many bytes we've sent
        ssize_t bytesleft = len; // how many we have left to send
        ssize_t n;

        while(total < len) {
            n = send(s, buf+total, bytesleft, 0);
            if (n == -1) { break; }
            total += n;
            bytesleft -= n;
        }

        return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
    }

    static int send_msg(const char *msg, const int childfd) {
        // if (const ssize_t sent = send(childfd, msg, strlen(msg), 0); sent == 0) {
        if (const ssize_t sent = sendall(childfd, msg, strlen(msg)); sent == -1) {
            perror(COMMAND_HANDLER "send failed");
            return -1;
        }

        return 0;
    }

public:
    static void handleCommand(const std::string& msgFromUser, const int childfd) {
        std::string firstPart, secondPart;
        if (const size_t pos = msgFromUser.find(' '); pos == std::string::npos) {
            firstPart = msgFromUser;
        } else {
            firstPart = msgFromUser.substr(0, pos);
            secondPart = msgFromUser.substr(pos + 1);
        }

        int commandCase = DEFAULT_CASE; // Default case
        if (commandToCase.contains(firstPart)) {
            commandCase = commandToCase.at(firstPart);
        }

        std::string error;
        std::string result;
        switch (commandCase) {
            case PS: {
                const auto& psFaResult = ProcessListHelper::getPsFa();

                if (!secondPart.empty()) {
                    // try to convert second part to a number
                    int pid;
                    try {
                        pid = std::stoi(secondPart);
                    } catch (const std::invalid_argument& ia) {
                        result += COMMAND_HANDLER + secondPart + " is not a number!\n";
                        break;
                    }

                    // find the line from psFaResult that starts with the pid
                    const auto& pidLine = [pid, psFaResult] {
                        std::istringstream iss(psFaResult);
                        std::string line;
                        while (std::getline(iss, line)) {
                            if (line.find(std::to_string(pid)) == 0) {
                                return line;
                            }
                        }
                        return std::string();
                    }();

                    if (pidLine.empty()) {
                        result += COMMAND_HANDLER + secondPart + " is not a valid pid!\n";
                        break;
                    }

                    // get the first line from the psFaResult
                    const size_t pos = psFaResult.find('\n');
                    const auto& firstLine = psFaResult.substr(0, pos);

                    result += firstLine + "\n" + pidLine;
                } else
                    result += ProcessListHelper::getPsFa();
            } break;

            case LS:
                result += ListFileHelper::getLsLa(secondPart);
                error = ListFileHelper::getError();
                if (!error.empty())
                    result += error;
                break;

            case EX: {
                if (secondPart.empty() || secondPart.find_first_not_of(' ') == std::string::npos) {
                    result += COMMAND_HANDLER "ex must contain a command";
                    break;
                }

                // Prevent hangning execution of interactive programs, e.g. vim
                secondPart.insert(0, "timeout 5s ");

                const std::vector<std::string> ex_commands = [secondPart] {
                    std::vector<std::string> args;
                    std::string arg;
                    std::istringstream iss(secondPart);

                    while (std::getline(iss, arg, '|')) {
                        args.push_back(arg);
                    }

                    return args;
                }();

                result += ExecutorHelper::execute(ex_commands);
            } break;

            default:
                result += errorWrongCommand + msgFromUser;
                break;
        }

        send_msg(result.c_str(), childfd);
        send_msg(endOfMessage, childfd);
    }

};

#endif //COMMANDHANDLER_H
