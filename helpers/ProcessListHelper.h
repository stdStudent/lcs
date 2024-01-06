//
// Created by Labour on 05.01.2024.
//

#ifndef PROCESSLISTHELPER_H
#define PROCESSLISTHELPER_H

#include <dirent.h>
#include <cstdio>
#include <cctype>
#include <sstream>
#include <string>

#define PS_MODULE "(ps module) "

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
            ss << pid << "\t" << comm << "\t\t\t" << state << "\t" << ppid << "\t" << pgrp << "\t" << session << "\t" << tty_nr << "\t" << tpgid << "\t" << minflt << "\n";
        }

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

        ss << "pid\tcomm\t\t\tstate\tppid\tpgrp\tsession\ttty_nr\ttpgid\tminflt\n";
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
