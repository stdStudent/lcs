//
// Created by Labour on 05.01.2024.
//

#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <fstream>
#include <unordered_map>
#include <sys/socket.h> // for socket

#include "Defines.h"
#include "ExecutorHelper.h"
#include "ListFilesHelper.h"
#include "ModificationTimeHelper.h"
#include "ProcessListHelper.h"
#include "ReceiveHelper.h"

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

    static int send_bin(const char *binary, const int childfd, const ssize_t bytes_read) {
        if (const ssize_t sent = sendall(childfd, binary, bytes_read); sent == -1) {
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

                // split secondPart by whitespace to a pair of strings
                const auto& [dl_file, dl_offset] = [secondPart] {
                    std::pair<std::string, std::string> args;
                    std::string arg;
                    std::istringstream iss(secondPart);

                    std::getline(iss, arg, ' ');
                    args.first = arg;

                    if (secondPart.find(' ') != std::string::npos) {
                        std::getline(iss, arg, ' ');
                        args.second = arg;
                    } else {
                        args.second = "";
                    }

                    return args;
                }();

                const auto& fileName = ConfigHelper::getDir() + "/" + dl_file;
                if (access(fileName.c_str(), F_OK) != 0) {
                    result += COMMAND_HANDLER "dl couldn't access the file";
                    break;
                }

                std::ifstream file(fileName, std::ios::binary);
                if (!file.is_open()) {
                    result += COMMAND_HANDLER "dl couldn't open the file";
                    break;
                }

                // if dl_offset is not empty and not a number, send error
                int file_offset = -1;
                if (!dl_offset.empty()) {
                    try {
                        file_offset = std::stoi(dl_offset);
                    } catch (const std::invalid_argument& ia) {
                        result += COMMAND_HANDLER + dl_offset + " is not a number!\n";
                        break;
                    }

                    if (file_offset < 0) {
                        result += COMMAND_HANDLER + dl_offset + " is not a valid offset!\n";
                        break;
                    }
                }

                long pageSize = sysconf(_SC_PAGESIZE);
                if (pageSize == -1) {
                    result += COMMAND_HANDLER "couldn't retrieve a pagesize from the filysystem";
                    break;
                }

                // get the size of a file to a variable
                struct stat filestatus{};
                stat(fileName.c_str(), &filestatus);
                auto& fileSize = filestatus.st_size; // in bytes

                // if file_offest is >= 0 and < fileSize, we need to send only a part of a file
                if (file_offset >= 0) {
                    if (file_offset > fileSize) {
                        result += COMMAND_HANDLER "invalid offset received: bigger than the file's size";
                        break;
                    }

                    file.seekg(file_offset);
                    if (!file.good()) {
                        result += COMMAND_HANDLER "couldn't seek to the offset";
                        break;
                    }

                    // change the fileSize to the size of a part of a file
                    fileSize -= file_offset;
                }

                char signalTransferFile[1024]{};
                sprintf(signalTransferFile, TRANSFER_FILE, dl_file.c_str());
                signalTransferFile[strlen(signalTransferFile)] = '\0';

                send_msg(signalTransferFile, childfd);
                ReceiveHelper::receiveData(childfd);

                long bytes_sent = 0;
                char buffer[pageSize];
                memset(buffer, 0, sizeof(buffer));
                while (file.readsome(buffer, pageSize) > 0) {
                    const auto& bytes_read = file.gcount();
                    send_bin(buffer, childfd, bytes_read);
                    bytes_sent += bytes_read;
                    // clear the buffer
                    memset(buffer, 0, sizeof(buffer));
                }

                if (bytes_sent != fileSize) {
                    // get formatter error string
                    std::stringstream err;
                    err << COMMAND_HANDLER
                        << "couldb't send the whole file " << fileName
                        << " (" << bytes_sent << "/" << fileSize << " bytes sent)";
                    perror(err.str().c_str());
                }

                file.close();
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

        send_msg(result.c_str(), childfd);
        send_msg(endOfMessage, childfd);
    }

};

#endif //COMMANDHANDLER_H
