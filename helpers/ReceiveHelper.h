//
// Created by Labour on 08.01.2024.
//

#ifndef RECEIVEHELPER_H
#define RECEIVEHELPER_H

#include <sys/socket.h>

#include "Defines.h"

class ReceiveHelper {
public:
    static std::vector<char> receiveData(const int sockfd, const int BUFSIZE = 1024) {
        char tempBuf[BUFSIZE];
        memset(tempBuf, 0, sizeof(tempBuf));

        const ssize_t received = recv(sockfd, tempBuf, BUFSIZE - 1, 0);
        if (received <= 0) {
            if (received == 0)
                printf(RECEIVE_HELPER "Connection closed by server\n");
            else
                perror(RECEIVE_HELPER "recv failed");

            // empty vector
            return {};
        }

        return std::vector(tempBuf, tempBuf + received);
    }
};

#endif //RECEIVEHELPER_H
