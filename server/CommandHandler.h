//
// Created by Labour on 05.01.2024.
//

#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <unordered_map>
#include <sys/socket.h> // for socket

#include "ProcessListHelper.h"

#define COMMAND_HANDLER "(command handler) "

#define DEFAULT_CASE 0
#define PS 1
#define LS 2

class CommandHandler {
    static inline const auto errorWrongCommand = "! Wrong command received: ";
    static inline const char* endOfMessage = "[[SERVICE:SIGNAL:END_OF_MESSAGE]]";

    const inline static std::unordered_map<std::string,int> commandToCase{
        {"default", DEFAULT_CASE},
        {"ps", PS},
        {"ls", LS}
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
        int commandCase = DEFAULT_CASE; // Default case
        if (commandToCase.contains(msgFromUser)) {
            commandCase = commandToCase.at(msgFromUser);
        }

        std::string result;
        switch (commandCase) {
            case PS:
                result = ProcessListHelper::getPsFa();
                break;

            case LS:
                result = "NOT IMPLEMENTED YET.";
                break;

            default:
                result = errorWrongCommand + msgFromUser;
                break;
        }

        send_msg(result.c_str(), childfd);
        send_msg(endOfMessage, childfd);
    }

};

#endif //COMMANDHANDLER_H
