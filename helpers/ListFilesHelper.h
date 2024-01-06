//
// Created by Labour on 06.01.2024.
//

#ifndef LISTFILESHELPER_H
#define LISTFILESHELPER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <cstdio>
#include <dirent.h>
#include <ctime>
#include <pwd.h>
#include <grp.h>
#include <string>

#include "Defines.h"

class ListFileHelper {
    static std::string getBinDir() {
        char buffer[1024];
        const ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (len == -1) {
            perror(LS_MODULE "readlink /proc/self/exe failed");
            exit(-1);
        }

        buffer[len] = '\0';
        auto binDir = std::string(buffer);

        // Remove the binary filename from the path
        if (const size_t lastSlashPos = binDir.find_last_of('/'); lastSlashPos != std::string::npos) {
            binDir = binDir.substr(0, lastSlashPos);
        }

        return binDir;
    }

    static bool mkdirNextToBin(const std::string& dirName) {
        // Get the directory of the binary
        std::string binDir = getBinDir();

        // Append the new directory name to the binary directory path
        binDir += "/" + dirName;

        // Check if the directory already exists
        struct stat st{};
        if (stat(binDir.c_str(), &st) != -1) {
            printf(LS_MODULE "Directory %s already exists\n", dirName.c_str());
            return false;
        }

        // Create the directory
        if (mkdir(binDir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0) {
            return true;
        }

        // An error occurred
        perror(LS_MODULE "mkdir");
        return false;
    }

public:
    static bool initServerDir() {
        return mkdirNextToBin(SERVER_DIR);
    }

    static bool initClientDir() {
        return mkdirNextToBin(CLIENT_DIR);
    }

	static std::string getLsLa(std::string& path) {
       DIR *thedirectory;
       dirent *thefile;
       struct stat thestat{};
       passwd *tf;
       group *gf;
       struct statvfs vfs{};

       off_t total_size = 0;

       std::string result;
       std::string server_path = getBinDir() + "/" + SERVER_DIR;

       // If path is empty, use the current working directory (SERVER_DIR)
       if (path.empty()) {
           path = server_path;
       } else {
           path = server_path + path;
       }

       // Get the block size
       if (statvfs(path.c_str(), &vfs) == -1) {
           perror("statvfs() error");
           return "";
       }

       thedirectory = opendir(path.c_str());

       while((thefile = readdir(thedirectory)) != nullptr) {
           char buf[512];
           sprintf(buf, "%s/%s", path.c_str(), thefile->d_name);
           lstat(buf, &thestat);

           std::string link_result;
           switch (thestat.st_mode & S_IFMT) {
               case S_IFBLK: result += "b"; break;
               case S_IFCHR: result += "c"; break;
               case S_IFDIR: result += "d"; break;
               case S_IFIFO: result += "p"; break;
               case S_IFLNK: {
                   result += "l";
                   char link_target[1024];
                   if (const ssize_t len = readlink(buf, link_target, sizeof(link_target) - 1); len != -1) {
                      link_target[len] = '\0';
                      link_result += " -> ";
                      link_result += link_target;
                   }
                   break;
               }
               case S_IFSOCK: result += "s"; break;
               default:       result += "-"; break;
           }

           result += (thestat.st_mode & S_IRUSR) ? "r" : "-";
           result += (thestat.st_mode & S_IWUSR) ? "w" : "-";
           result += (thestat.st_mode & S_IXUSR) ? "x" : "-";
           result += (thestat.st_mode & S_IRGRP) ? "r" : "-";
           result += (thestat.st_mode & S_IWGRP) ? "w" : "-";
           result += (thestat.st_mode & S_IXGRP) ? "x" : "-";
           result += (thestat.st_mode & S_IROTH) ? "r" : "-";
           result += (thestat.st_mode & S_IWOTH) ? "w" : "-";
           result += (thestat.st_mode & S_IXOTH) ? "x" : "-";

           result += "\t";
           result += std::to_string(thestat.st_nlink);

           tf = getpwuid(thestat.st_uid);
           result += "\t";
           result += tf->pw_name;

           gf = getgrgid(thestat.st_gid);
           result += "\t";
           result += gf->gr_name;

           result += "\t";
           result += std::to_string(thestat.st_size);

           // Print the date in the format "Dec 13 11:55"
           char date[20];
           strftime(date, sizeof(date), "%b %d %H:%M", localtime(&thestat.st_mtime));
           result += "\t";
           result += date;

           result += "\t";
           result += thefile->d_name;
           result += link_result;
           result += "\n";

           total_size += thestat.st_size;
       }

       result += "\ntotal ";
       result += std::to_string(total_size / vfs.f_bsize);

       closedir(thedirectory);

       return result;
    }
};

#endif //LISTFILESHELPER_H
