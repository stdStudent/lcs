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
    static std::string print_status(const long tgid) {
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
            ss << std::setw(10) << pid << std::setw(20) << comm << std::setw(10) << state << std::setw(10) << ppid << std::setw(10) << pgrp << std::setw(10) << session << std::setw(10) << tty_nr << std::setw(10) << tpgid << std::setw(12) << minflt;
        }

        std::string cmdline_path = "/proc/" + std::to_string(tgid) + "/cmdline";
        if (std::ifstream cmdlinef(cmdline_path); cmdlinef) {
            std::string arg;
            while (std::getline(cmdlinef, arg, '\0')) {
                ss << ' ' << arg;
            }
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

        ss << std::setw(10) << "pid" << std::setw(20) << "comm" << std::setw(10) << "state" << std::setw(10) << "ppid" << std::setw(10) << "pgrp" << std::setw(10) << "session" << std::setw(10) << "tty_nr" << std::setw(10) << "tpgid" << std::setw(12) << "minflt" << ' ' << "cmdline\n";
        while ((ent = readdir(proc))) {
            if (!isdigit(*ent->d_name))
                continue;

            const long tgid = strtol(ent->d_name, nullptr, 10);

            ss << print_status(tgid);
        }

        closedir(proc);

        return ss.str();
    }
};

#endif //PROCESSLISTHELPER_H
