//
// Created by Labour on 05.01.2024.
//

#include "Server.h"

void usage() {
    printf("syntax: server <port>\n");
    printf("sample: server 1234\n");
}

int main(const int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return -1;
    }

    Server s(argv[optind]);
    return s.start();
}
