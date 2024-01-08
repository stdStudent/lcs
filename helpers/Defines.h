//
// Created by Labour on 06.01.2024.
//

#ifndef DEFINES_H
#define DEFINES_H

#define USER_LOG "(me) "
#define SERVER_LOG "(server) "
#define COMMAND_HANDLER "(command handler) "
#define PS_MODULE "(ps module) "
#define LS_MODULE "(ls module) "
#define EX_MODULE "(ex module)"
#define RECEIVE_HELPER "(receive helper)"
#define OUTPUT_CAPTURER_HELPER "(output captuter helper)"

#define END_OF_MESSAGE "[[SERVICE:SIGNAL:END_OF_MESSAGE]]"
#define TRANSFER_FILE "[[SERVICE:SIGNAL:TRANSFER_FILE:FILE_NAME:%s]]"
#define TRANSFER_FILE_CUTTED "[[SERVICE:SIGNAL:TRANSFER_FILE:FILE_NAME:"
#define TRANSFER_FILE_OK "[[SERVICE:SIGNAL:TRANSFER_FILE:OK]]"

#define CFG_GROUP_NAME "network"

#define DEFAULT_CASE 0
#define PS 1
#define LS 2
#define EX 3
#define DL 4

#endif //DEFINES_H
