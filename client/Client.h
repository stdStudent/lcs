//
// Created by Labour on 05.01.2024.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <cstdio> // for perror
#include <cstring> // for memset
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket

#include "Defines.h"
#include "ConfigHelper.h"
#include "ListFilesHelper.h"

class Client {
    int port = -1;
    char* ip_addr = nullptr;

    const std::string endOfMessage = END_OF_MESSAGE;

public:
    Client() : port(ConfigHelper::getPort()), ip_addr(strdup(ConfigHelper::getIp().c_str())) {
        printf(USER_LOG "Read ip: %s\n", ip_addr);
        printf(USER_LOG "Read port: %d\n", port);
        if (ListFileHelper::initDir()) {
            printf(USER_LOG "Initialized the client's directory\n");
        }
    };

    int start() const {
        const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            perror(USER_LOG "socket failed");
            return -1;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip_addr);
        memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

        if (const int res = connect(sockfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(struct sockaddr)); res == -1) {
            perror(USER_LOG "connect failed");
            return -1;
        }

        printf(USER_LOG "connected\n");

        while (true) {
            static constexpr int BUFSIZE = 1024;
            char buf[BUFSIZE];

            printf("> ");
            fgets(buf, BUFSIZE, stdin); // Read a line from stdin
            buf[strcspn(buf,"\n")] = 0; // Remove carret return from the user's input

            if (strcmp(buf, "") == 0) continue; // Skip if the input is empty
            if (strcmp(buf, "quit") == 0) break; // Quit (close the connection)

            if (const ssize_t sent = send(sockfd, buf, strlen(buf), 0); sent == 0) {
                perror(USER_LOG "send failed");
                break;
            }

            std::string response;
            char tempBuf[BUFSIZE];
            do {
                const ssize_t received = recv(sockfd, tempBuf, BUFSIZE - 1, 0);
                if (received <= 0) {
                    if (received == 0)
                        printf(USER_LOG "Connection closed by server\n");
                    else
                        perror(USER_LOG "recv failed");

                    break;
                }

                tempBuf[received] = '\0';
                response += tempBuf;
            } while (response.find(endOfMessage) == std::string::npos);

            // Remove service endOfMessage from the response only if it is at the very end
            if (!response.empty() && response.substr(response.size() - endOfMessage.length()) == endOfMessage)
                response.erase(response.size() - endOfMessage.length());

            // Remove trailing carriage return
            if (!response.empty() && response[response.size() - 1] == '\n')
                response.erase(response.size() - 1);

            printf(SERVER_LOG "\n%s\n", response.c_str());
        }

        close(sockfd);
        return 0;
    }
};

#endif //CLIENT_H
