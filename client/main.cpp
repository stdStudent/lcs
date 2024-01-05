//
// Created by Labour on 05.01.2024.
//

#include <cstdio> // for perror
#include <cstring> // for memset
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket
#include <cstdlib> // char* to int

#define USER_LOG "(me) "
#define SERVER_LOG "(server) "

void usage() {
    printf("syntax: client <host> <port>\n");
    printf("sample: client 127.0.0.1 1234\n");
}

int main(const int argc, char *argv[]) {
    if (argc != 3) {
        usage();
        return -1;
    }

    const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror(USER_LOG "socket failed");
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);
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

        if (strcmp(buf, "quit") == 0) break;

        if (const ssize_t sent = send(sockfd, buf, strlen(buf), 0); sent == 0) {
            perror(USER_LOG "send failed");
            break;
        }

        const ssize_t received = recv(sockfd, buf, BUFSIZE - 1, 0);
        if (received == 0 || received == -1) {
            perror(USER_LOG "recv failed");
            break;
        }

        buf[received] = '\0';
        printf(SERVER_LOG "%s\n", buf);
    }

    close(sockfd);
    return 0;
}
