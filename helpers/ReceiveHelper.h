//
// Created by Labour on 08.01.2024.
//

#ifndef RECEIVEHELPER_H
#define RECEIVEHELPER_H

#include <string>
#include <sys/socket.h>

#include "Defines.h"

class ReceiveHelper {
public:
    static std::string receiveData(const int sockfd, const int BUFSIZE = 1024) {
        char tempBuf[BUFSIZE];
        const ssize_t received = recv(sockfd, tempBuf, BUFSIZE - 1, 0);
        if (received <= 0) {
            if (received == 0)
                printf(RECEIVE_HELPER "Connection closed by server\n");
            else
                perror(RECEIVE_HELPER "recv failed");

            return "";
        }

        tempBuf[received] = '\0';
        return std::string(tempBuf);
    }
};

#endif //RECEIVEHELPER_H
