//
// Created by Labour on 05.01.2024.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <cstdio> // for perror
#include <cstring> // for memset
#include <fstream>
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket

#include "Defines.h"
#include "ConfigHelper.h"
#include "ListFilesHelper.h"
#include "ReceiveHelper.h"

class Client {
    int port = -1;
    char* ip_addr = nullptr;

    const std::string endOfMessage = END_OF_MESSAGE;
    const std::string transferFile = TRANSFER_FILE_CUTTED;
    const std::string transferFileOk = TRANSFER_FILE_OK;

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

            bool saveFile = false;
            std::ofstream file;

            std::string response;
            int cnt = 0;
            bool server_log_prited = false;
            const auto& default_max_cnt = 4;
            int max_cnt = default_max_cnt;

            do {
                auto data = ReceiveHelper::receiveData(sockfd, BUFSIZE);

                if (data.starts_with(transferFile)) {
                    // extract the file name from the string "[[SERVICE:SIGNAL:FILE_NAME:%s]]" between FILE_NAME: and ]]
                    const size_t startPos = data.find("FILE_NAME:") + 10;
                    const size_t endPos = data.rfind("]]");

                    std::string fileName;
                    if (startPos != std::string::npos && endPos != std::string::npos) {
                        fileName = data.substr(startPos, endPos - startPos);
                    }

                    // open file to write to
                    file.open(ConfigHelper::getDir() + "/" + fileName, std::ios::out | std::ios::binary);

                    saveFile = true;

                    if (const ssize_t sent = send(sockfd, transferFileOk.c_str(), transferFileOk.length(), 0); sent == 0) {
                        perror(USER_LOG "send failed");
                        break;
                    }

                    continue;
                }

                response += data;
                ++cnt;

                const auto& foundEOM = response.find(endOfMessage) != std::string::npos;

                if (cnt == max_cnt && foundEOM == false && max_cnt != default_max_cnt)
                    max_cnt = default_max_cnt;

                if (cnt == max_cnt)
                    if (const auto p = response.find('['); p != std::string::npos)
                        max_cnt *= 2;


                if (cnt == max_cnt || foundEOM) {
                    // Remove service endOfMessage from the response only if it is at the very end
                    if (!response.empty() && response.substr(response.size() - endOfMessage.length()) == endOfMessage)
                        response.erase(response.size() - endOfMessage.length());

                    // Remove trailing carriage return
                    if (saveFile == false)
                        if (!response.empty() && response[response.size() - 1] == '\n')
                            response.erase(response.size() - 1);

                    if (saveFile == true) {
                        // write response to file
                        file << response;
                    } else {
                        if (server_log_prited == false) {
                            printf(SERVER_LOG "\n%s\n", response.c_str());
                            server_log_prited = true;
                        } else {
                            printf(SERVER_LOG "%s\n", response.c_str());
                        }
                    }

                    if (foundEOM) {
                        break;
                    }

                    response.clear();
                    cnt = 0;
                }
            } while (true);

            file.close();
        }

        close(sockfd);
        return 0;
    }
};

#endif //CLIENT_H
