//
// Created by Labour on 05.01.2024.
//

#ifndef SERVER_H
#define SERVER_H

#include <map> // for map
#include <cstdio> // for perror
#include <cstring> // for memset
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket
#include <pthread.h> // for multithread
#include <string> // for strings

#include "Defines.h"
#include "ConfigHelper.h"
#include "CommandHandler.h"
#include "ListFilesHelper.h"

class Server {
    int port;
    pthread_mutex_t mutx;
    int client[10];
    int client_num = 0;
    bool option = false;
    std::map<int, std::string> clientIdentifiers;

    struct ClientConnectArgs {
        Server* server;
        int childfd;
    };

    // ReSharper disable once CppFunctionDoesntReturnValue
    /**
     * Handles a client connection.
     *
     * This function runs indefinitely in a separate thread, performing tasks related to handling a client connection.
     * It does not return a value because its job is to run continuously until the connection is closed. Once the connection
     * is closed, the function simply ends, allowing the operating system to clean up the resources associated with the thread.
     *
     * @param argument A pointer to an int representing the file descriptor for the client connection.
     */
    static void *client_connect(void *argument) {
        const auto* args = static_cast<ClientConnectArgs*>(argument);
        Server* server = args->server;
        const int childfd = args->childfd;

        //const int childfd = *static_cast<int *>(argument);
        while (true) {
            static constexpr int BUFSIZE = 1024;
            char buf[BUFSIZE];
            const ssize_t received = recv(childfd, buf, BUFSIZE - 1, 0);
            if (received == 0 || received == -1) {
                perror(SERVER_LOG "recv failed");
                break;
            }

            buf[received] = '\0';
            CommandHandler::handleCommand(buf, childfd);

            printf("(%s) %s\n", server->clientIdentifiers[childfd].c_str(), buf);
        }

        pthread_mutex_lock(&server->mutx);
        for (int i = 0; i < server->client_num; i++) {
            if (childfd == server->client[i]) {
                for (; i < server->client_num - 1; i++)
                    server->client[i] = server->client[i + 1];
                break;
            }
        }

        server->client_num--;
        pthread_mutex_unlock(&server->mutx);
    }

public:
    Server() : port(ConfigHelper::getPort()) {
        printf(SERVER_LOG "Read port: %d\n", port);
        if (ListFileHelper::initServerDir()) {
            printf(SERVER_LOG "Initialized the server's directory\n");
        }
    };

    int start() {
        if (pthread_mutex_init(&mutx,nullptr)) {
            printf("Mutex init error!\n");
            return -1;
        }

        const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            perror(SERVER_LOG "socket failed");
            return -1;
        }

        pthread_t pthread;

        constexpr int optval = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

        int res = bind(sockfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(struct sockaddr));
        if (res == -1) {
            perror(SERVER_LOG "bind failed");
            return -1;
        }

        res = listen(sockfd, 2);
        if (res == -1) {
            perror(SERVER_LOG "listen failed");
            return -1;
        }

        while (true) {
            sockaddr_in address{};
            socklen_t clientLength = sizeof(sockaddr);

            int childfd = accept(sockfd, reinterpret_cast<struct sockaddr *>(&address), &clientLength);
            if (childfd < 0) {
                perror(SERVER_LOG "ERROR on accept");
                break;
            }

            // Set id for a user
            const std::string identifier = "user" + std::to_string(childfd);
            clientIdentifiers[childfd] = identifier;

            pthread_mutex_lock(&mutx);
            client[client_num++] = childfd;
            pthread_mutex_unlock(&mutx);

            // Provide the server and the childfd to the pthread to avoid global variables
            ClientConnectArgs args{};
            args.server = this;
            args.childfd = childfd;

            pthread_create(&pthread,nullptr, client_connect, &args);
            printf(SERVER_LOG "%s connected\n", clientIdentifiers[childfd].c_str());
        }

        close(sockfd);
        return 0;
    }
};

#endif //SERVER_H
