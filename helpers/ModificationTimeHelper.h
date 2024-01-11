//
// Created by Labour on 11.01.2024.
//

#ifndef MODIFICATIONTIMEHELPER_H
#define MODIFICATIONTIMEHELPER_H

#include <sys/stat.h>
#include <sstream>
#include <string>

class ModificationTimeHelper {
public:
    static std::string getModificationTime(const std::string& file) {
        const auto& filePath = ConfigHelper::getDir() + '/' + file;

        struct stat buffer{};
        if (stat(filePath.c_str(), &buffer) == -1) {
            return "couldn't get the modification time. Does the file exist?";
        }

        std::ostringstream oss;

        char atime[100] = {};
        const tm* atime_tm = localtime(&buffer.st_atime);
        strftime(atime, sizeof(atime), "%H:%M:%S %d-%m-%Y", atime_tm);
        oss << "Access Time:       " << atime << "\n";

        char mtime[100] = {};
        const tm* mtime_tm = localtime(&buffer.st_mtime);
        strftime(mtime, sizeof(mtime), "%H:%M:%S %d-%m-%Y", mtime_tm);
        oss << "Modification Time: " << mtime << "\n";

        char ctime[100] = {};
        const tm* ctime_tm = localtime(&buffer.st_ctime);
        strftime(ctime, sizeof(ctime), "%H:%M:%S %d-%m-%Y", ctime_tm);
        oss << "Change Time:       " << ctime;

        return oss.str();
    }
};

#endif //MODIFICATIONTIMEHELPER_H
