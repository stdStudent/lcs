//
// Created by Labour on 05.01.2024.
//

#include "Client.h"

void usage() {
    printf("syntax: client <host> <port>\n");
    printf("sample: client 127.0.0.1 1234\n");
}

int main(const int argc, char *argv[]) {
    if (argc != 3) {
        usage();
        return -1;
    }

    const Client c(argv[2], argv[1]);
    return c.start();
}
