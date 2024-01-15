//
// Created by Labour on 15.01.2024.
//

#ifndef UPLOADFILEHELPER_H
#define UPLOADFILEHELPER_H

#include <string>
#include <cstring> // for memset
#include <sys/socket.h> // for socket
#include <unistd.h> // for access

#include "Defines.h"
#include "TransferFileHelper.h"
#include "ReceiveHelper.h"

class UploadFileHelper {
    static inline const std::string endOfMessage = END_OF_MESSAGE;

public:
    class Sender {
    public:
        static bool uploadTo(const int sockfd, char* buf, const int BUFSIZE) {
            // "ul somefile.txt"
            // get filename from the user's input
            const auto& fileName = std::string(buf).substr(3);

            // get path to the file
            const auto& filePath = ConfigHelper::getDir() + '/' + fileName;

            // check if the file accessible
            if (access(filePath.c_str(), F_OK) != 0) {
                printf(UPLOAD_FILE_HELPER_SENDER "couldn't access the file\n");
                return false;
            }

            if (const ssize_t sent = send(sockfd, buf, strlen(buf), 0); sent == 0) {
                perror(UPLOAD_FILE_HELPER_SENDER "send failed");
                return false;
            }

            std::string result;
            if (const auto& status = TransferFileHelper::transferFile(sockfd, fileName, "0", result, false); status == false) {
                printf(UPLOAD_FILE_HELPER_SENDER "failed due to the error:\n%s\n", result.c_str());
                return false;
            }

            // send EOM to the server
            if (const ssize_t sent = send(sockfd, endOfMessage.c_str(), endOfMessage.length(), 0); sent == 0) {
                perror(UPLOAD_FILE_HELPER_SENDER "send failed");
                return false;
            }

            // receive response from the server
            const auto& [uploadStatusResponse, status] = ReceiveHelper::receiveData(sockfd, BUFSIZE);
            if (status == false) {
                printf(UPLOAD_FILE_HELPER_SENDER "failed to receive upload status response from the server\n");
                return false;
            }

            // remove endOfMessage from the uploadStatusResponse
            auto usr = std::string(uploadStatusResponse.begin(), uploadStatusResponse.end());
            usr.erase(usr.find(endOfMessage), endOfMessage.length());

            // print upload status response if any
            if (!usr.empty()) printf(UPLOAD_FILE_HELPER_SENDER "%s\n", usr.c_str());

            // clear buffer and continue
            memset(buf, 0, BUFSIZE);
            return true;
        }
    };

    class Recipient {
    public:
        static void downloadFrom(const int childfd, const std::string& secondPart, std::string& result) {
            // open file to write to
            std::ofstream file;
            const auto& filePath = ConfigHelper::getDir() + '/' + secondPart;
            file.open(filePath, std::ios::out | std::ios::binary);

            const auto& BUFSIZE = 1024;

            std::string response;
            std::vector<char> bytesResponse;
            const auto& default_max_cnt = 4;
            int max_cnt = default_max_cnt;
            int cnt = 0;

            const auto& eom = std::string(endOfMessage);

            do {
                const auto& [bytes, status] = ReceiveHelper::receiveData(childfd, BUFSIZE);
                if (!status) {
                    result += UPLOAD_FILE_HELPER_RECIPIENT "failed to send data";
                    break;
                }

                response += std::string(bytes.begin(), bytes.end());
                bytesResponse.insert(bytesResponse.end(), bytes.begin(), bytes.end());
                ++cnt;

                const auto& foundEOM = response.find(eom) != std::string::npos;

                if (cnt == max_cnt && foundEOM == false && max_cnt != default_max_cnt)
                    max_cnt = default_max_cnt;

                if (cnt == max_cnt)
                    if (const auto p = response.find('['); p != std::string::npos)
                        max_cnt *= 2;

                if (cnt == max_cnt || foundEOM) {
                    // Remove service endOfMessage from the bytesResponse only if it is at the very end
                    if (!bytesResponse.empty() && bytesResponse[bytesResponse.size() - eom.length()] == eom[0])
                        bytesResponse.erase(bytesResponse.end() - eom.length(), bytesResponse.end());

                    file.write(bytesResponse.data(), bytesResponse.size());

                    response.clear();
                    bytesResponse.clear();
                    cnt = 0;

                    if (foundEOM) break;
                }
            } while (true);

            file.close();
        }
    };

};

#endif //UPLOADFILEHELPER_H
