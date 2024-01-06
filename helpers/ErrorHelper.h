//
// Created by Labour on 06.01.2024.
//

#ifndef ERRORHELPER_H
#define ERRORHELPER_H

#include <cstring>
#include <iostream>
#include <string>

class ErrorHelper {
public:
    static std::string perror(const std::string& userMsg) {
        const auto errMsg = std::string(strerror(errno));
        std::cerr << userMsg << ": " << errMsg << '\n';
        return errMsg;
    }
};

#endif //ERRORHELPER_H
