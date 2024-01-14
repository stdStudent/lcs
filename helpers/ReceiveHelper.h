//
// Created by Labour on 08.01.2024.
//

#ifndef RECEIVEHELPER_H
#define RECEIVEHELPER_H

#include <sys/socket.h>

#include "Defines.h"

class ReceiveHelper {
public:
    /**
     * \return bytes and status - true if connection is alive, false if connection is closed
     */
    static std::pair<std::vector<char>, bool> receiveData(const int sockfd, const int BUFSIZE = 1024) {
        char tempBuf[BUFSIZE];
        memset(tempBuf, 0, sizeof(tempBuf));

        const ssize_t received = recv(sockfd, tempBuf, BUFSIZE - 1, 0);
        if (received <= 0) {
            if (received == 0) {
                printf(RECEIVE_HELPER "Connection closed by other side\n");
                return {{}, false};
            } else
                perror(RECEIVE_HELPER "recv failed");

            // return empty vector and true because connection is alive
            return {{}, true};
        }

        return {std::vector(tempBuf, tempBuf + received), true};
    }
};

#endif //RECEIVEHELPER_H
