//
// Created by Labour on 15.01.2024.
//

#ifndef SENDHELPER_H
#define SENDHELPER_H

#include <cstdio>
#include <cstring>
#include <sys/socket.h> // for socket

#include "Defines.h"

class SendHelper {
    static ssize_t send_all(const int s, const char *buf, const ssize_t len) {
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

public:
    static int send_msg(const char *msg, const int childfd) {
        // if (const ssize_t sent = send(childfd, msg, strlen(msg), 0); sent == 0) {
        if (const ssize_t sent = send_all(childfd, msg, strlen(msg)); sent == -1) {
            perror(COMMAND_HANDLER "send failed");
            return -1;
        }

        return 0;
    }

    static int send_bin(const char *binary, const int childfd, const ssize_t bytes_read) {
        if (const ssize_t sent = send_all(childfd, binary, bytes_read); sent == -1) {
            perror(COMMAND_HANDLER "send failed");
            return -1;
        }

        return 0;
    }
};

#endif //SENDHELPER_H
