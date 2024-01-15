//
// Created by Labour on 05.01.2024.
//

#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <fstream>
#include <unordered_map>

#include "Defines.h"
#include "ExecutorHelper.h"
#include "ListFilesHelper.h"
#include "ModificationTimeHelper.h"
#include "ProcessListHelper.h"
#include "SendHelper.h"
#include "TransferFileHelper.h"

class CommandHandler {
    static inline const auto errorWrongCommand = "! Wrong command received: ";
    static inline const char* endOfMessage = END_OF_MESSAGE;

    const inline static std::unordered_map<std::string,int> commandToCase{
        {"default", DEFAULT_CASE},
        {"ps", PS},
        {"ls", LS},
        {"ex", EX},
        {"dl", DL},
        {"mt", MT}
    };

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
                    try {
                        std::stoi(secondPart);
                    } catch (const std::invalid_argument& ia) {
                        result += COMMAND_HANDLER + secondPart + " is not a number!\n";
                        break;
                    }

                    // find the line from psFaResult that starts with the pid
                    const auto& pidLine = [secondPart, psFaResult] {
                        const std::string whitespace = " \t";
                        std::istringstream iss(psFaResult);
                        std::string line;
                        while (std::getline(iss, line)) {
                            const auto strBegin = line.find_first_not_of(whitespace);
                            if (strBegin != std::string::npos)
                                line = line.substr(strBegin);

                            if (line.starts_with(secondPart + " ")) {
                                line.insert(0, strBegin, ' ');
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

            case DL: {
                if (secondPart.empty()) {
                    result += COMMAND_HANDLER "dl must contain at least a file name or a path to file";
                    break;
                }

                const auto& [dl_file, dl_offset] = TransferFileHelper::extractDlArgs(secondPart);

                if (const auto& status = TransferFileHelper::transferFile(childfd, dl_file, dl_offset, result); status == false)
                    break;
            } break;

            case MT:
                if (secondPart.empty()) {
                    result += COMMAND_HANDLER "mt must contain a file name or a path to file";
                    break;
                }

                result += ModificationTimeHelper::getModificationTime(secondPart);

                break;

            default:
                result += errorWrongCommand + msgFromUser;
                break;
        }

        SendHelper::send_msg(result.c_str(), childfd);
        SendHelper::send_msg(endOfMessage, childfd);
    }

};

#endif //COMMANDHANDLER_H
