//
// Created by Labour on 05.01.2024.
//

#ifndef PROCESSLISTHELPER_H
#define PROCESSLISTHELPER_H

#include <dirent.h>
#include <cstdio>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

#include "Defines.h"

class ProcessListHelper {
    static inline constexpr int fileDescCutSize = 39; // till the start of the file descriptor number
    static inline constexpr int fileDescsMaxWidth = 43;
    static inline constexpr int cmdLineMaxWidth = 33;

    static void appendCmdLine(const long& tgid, std::stringstream& ss) {
        std::string cmdline_path = "/proc/" + std::to_string(tgid) + "/cmdline";
        if (std::ifstream cmdlinef(cmdline_path); cmdlinef) {
            std::string arg;
            std::string cmdline_result;
            while (std::getline(cmdlinef, arg, '\0')) {
                cmdline_result += ' ' + arg;
            }

            if (cmdline_result.size() > cmdLineMaxWidth) {
                cmdline_result = cmdline_result.substr(0, cmdLineMaxWidth - 3);
                cmdline_result += "...";
            }

            ss << cmdline_result;
        }
    }

    static void appendFileDescriptors(const long& tgid, std::stringstream& ss) {
        std::string fileDescPath = "/proc/" + std::to_string(tgid) + "/fd";
        auto fileDescriptors = ListFileHelper::getLsLa(fileDescPath, true);

        // skip two lines from the begginig
        fileDescriptors = fileDescriptors.substr(fileDescriptors.find('\n') + 1);
        fileDescriptors = fileDescriptors.substr(fileDescriptors.find('\n') + 1);

        // skip two lines from the end
        fileDescriptors = fileDescriptors.substr(0, fileDescriptors.find_last_of('\n'));
        fileDescriptors = fileDescriptors.substr(0, fileDescriptors.find_last_of('\n'));

        // split by lines
        std::stringstream fileDescriptorsStream(fileDescriptors);
        std::string tmp;
        std::vector<std::string> lines;
        while (std::getline(fileDescriptorsStream, tmp, '\n')) {
            lines.push_back(tmp);
        }

        // from each line delete symbols until actual file descriptors
        std::string resultFileDescs;
        for (auto& line : lines) {
            if (line.size() > fileDescCutSize)
                line.erase(0, fileDescCutSize);
            else
                line.clear();
            resultFileDescs += line + ", ";
        }

        // delete last ", "
        resultFileDescs.erase(resultFileDescs.size() - 2, 2);

        // replace '/t' with ' '
        std::replace(resultFileDescs.begin(), resultFileDescs.end(), '\t', ' ');

        if (resultFileDescs.size() > fileDescsMaxWidth) {
            resultFileDescs = resultFileDescs.substr(0, fileDescsMaxWidth - 3);
            resultFileDescs += "...";
        }

        ss << ' ' << resultFileDescs;
    }

    static std::string print_status(const long tgid, const bool& isSu) {
        char path[40];
        int pid, ppid, pgrp, session, tty_nr, tpgid;
        unsigned long minflt;
        char state;
        std::stringstream ss;

        snprintf(path, 40, "/proc/%ld/stat", tgid);
        FILE *statf = fopen(path, "r");
        if (!statf)
            return "";

        if (char line[100]; fgets(line, 100, statf)) {
            char comm[20];
            sscanf(line, "%d %s %c %d %d %d %d %d %lu", &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &minflt);
            ss << std::setw(10) << pid << std::setw(20) << comm << std::setw(10) << state << std::setw(10) << ppid << std::setw(10) << pgrp << std::setw(10) << session << std::setw(10) << tty_nr << std::setw(10) << tpgid << std::setw(12) << minflt << std::setw(cmdLineMaxWidth);
        }

        appendCmdLine(tgid, ss);

        if (isSu) {
            appendFileDescriptors(tgid, ss);
        } else {
            ss << ' ' << "authorisation denied";
        }

        ss << "\n";
        fclose(statf);
        return ss.str();
    }

public:
    static std::string getPsFa() {
        DIR* proc = opendir("/proc");
        dirent* ent;
        std::stringstream ss;

        if (proc == nullptr) {
            perror(PS_MODULE "opendir(/proc)");
            return "";
        }

        const auto& isSu = getuid() ? false : true;
        ss << std::setw(10) << "pid" << std::setw(20) << "comm" << std::setw(10) << "state" << std::setw(10) << "ppid" << std::setw(10) << "pgrp" << std::setw(10) << "session" << std::setw(10) << "tty_nr" << std::setw(10) << "tpgid" << std::setw(12) << "minflt" << std::setw(cmdLineMaxWidth) << "cmdline" << ' ' << "filedescs" << '\n';

        while ((ent = readdir(proc))) {
            if (!isdigit(*ent->d_name))
                continue;

            const long tgid = strtol(ent->d_name, nullptr, 10);

            ss << print_status(tgid, isSu);
        }

        closedir(proc);

        return ss.str();
    }
};

#endif //PROCESSLISTHELPER_H
