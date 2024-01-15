//
// Created by Labour on 15.01.2024.
//

#ifndef TRANFERFILEHELPER_H
#define TRANFERFILEHELPER_H

#include <string>
#include <sstream>
#include <fstream>
#include <sys/stat.h> // for stat
#include <unistd.h> // for access

#include "Defines.h"
#include "ConfigHelper.h"
#include "SendHelper.h"
#include "ReceiveHelper.h"

class TransferFileHelper {
public:
    /**
     * \brief Splits rawArgs by whitespace to a pair of strings
     * \param rawArgs Example: "somefile.txt 256"
     * \return Example: {"somefile.txt", "256"}
     */
    static std::pair<std::string, std::string> extractDlArgs(const std::string& rawArgs) {
        std::pair<std::string, std::string> args;
        std::string arg;
        std::istringstream iss(rawArgs);

        std::getline(iss, arg, ' ');
        args.first = arg;

        if (rawArgs.find(' ') != std::string::npos) {
            std::getline(iss, arg, ' ');
            args.second = arg;
        } else {
            args.second = "";
        }

        return args;
    }

    static bool transferFile(
        const int childfd,
        const std::string& dl_file,
        const std::string& dl_offset,
        std::string& result,
        const bool& signalsEnabled = true
    ) {
        const auto& fileName = ConfigHelper::getDir() + "/" + dl_file;
        if (access(fileName.c_str(), F_OK) != 0) {
            result += TRANSFER_FILE_HELPER "couldn't access the file";
            return false;
        }

        std::ifstream file(fileName, std::ios::binary);
        if (!file.is_open()) {
            result += TRANSFER_FILE_HELPER "couldn't open the file";
            return false;
        }

        // if dl_offset is not empty and not a number, send error
        int file_offset = -1;
        if (!dl_offset.empty()) {
            try {
                file_offset = std::stoi(dl_offset);
            } catch (const std::invalid_argument& ia) {
                result += TRANSFER_FILE_HELPER + dl_offset + " is not a number!\n";
                return false;
            }

            if (file_offset < 0) {
                result += TRANSFER_FILE_HELPER + dl_offset + " is not a valid offset!\n";
                return false;
            }
        }

        long pageSize = sysconf(_SC_PAGESIZE);
        if (pageSize == -1) {
            result += TRANSFER_FILE_HELPER "couldn't retrieve a pagesize from the filesystem";
            return false;
        }

        // get the size of a file to a variable
        struct stat filestatus{};
        stat(fileName.c_str(), &filestatus);
        auto& fileSize = filestatus.st_size; // in bytes

        // if file_offest is >= 0 and < fileSize, we need to send only a part of a file
        if (file_offset >= 0) {
            if (file_offset > fileSize) {
                result += TRANSFER_FILE_HELPER "invalid offset received: bigger than the file's size";
                return false;
            }

            file.seekg(file_offset);
            if (!file.good()) {
                result += TRANSFER_FILE_HELPER "couldn't seek to the offset";
                return false;
            }

            // change the fileSize to the size of a part of a file
            fileSize -= file_offset;
        }

        if (signalsEnabled) {
            char signalTransferFile[1024]{};
            sprintf(signalTransferFile, TRANSFER_FILE, dl_file.c_str());
            signalTransferFile[strlen(signalTransferFile)] = '\0';

            SendHelper::send_msg(signalTransferFile, childfd);
            ReceiveHelper::receiveData(childfd);
        }

        long bytes_sent = 0;
        char buffer[pageSize];
        memset(buffer, 0, sizeof(buffer));
        while (file.readsome(buffer, pageSize) > 0) {
            const auto& bytes_read = file.gcount();
            SendHelper::send_bin(buffer, childfd, bytes_read);
            bytes_sent += bytes_read;
            // clear the buffer
            memset(buffer, 0, sizeof(buffer));
        }

        if (bytes_sent != fileSize) {
            // get formatter error string
            std::stringstream err;
            err << TRANSFER_FILE_HELPER
                << "couldb't send the whole file " << fileName
                << " (" << bytes_sent << "/" << fileSize << " bytes sent)";
            perror(err.str().c_str());
        }

        file.close();
        return true;
    }
};

#endif //TRANFERFILEHELPER_H
