//
// Created by Labour on 05.01.2024.
//

#ifndef CONFIGHELPER_H
#define CONFIGHELPER_H

#include <iostream>
#include <libconfig.h++>

using namespace libconfig;

class ConfigHelper {
public:
    static inline const std::string default_path = "config.cfg";

    static char* getIp(const std::string &configPath = default_path) {
        return getConfigValue<std::string>(configPath, "network", "ip").data();
    }

    static int getPort(const std::string &configPath = default_path) {
        return getConfigValue<int>(configPath, "network", "port");
    }

private:
    template<typename T>
    static T getConfigValue(const std::string &configPath, const std::string &groupName,
                            const std::string &settingName) {
        Config cfg;
        try {
            cfg.readFile(configPath.c_str());
        } catch (const FileIOException &fioex) {
            std::cerr << "I/O error while reading file." << std::endl;
            exit(EXIT_FAILURE);
        } catch (const ParseException &pex) {
            std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                    << " - " << pex.getError() << std::endl;
            exit(EXIT_FAILURE);
        }

        const Setting &root = cfg.getRoot();
        const Setting &group = root[groupName.c_str()];
        T value;
        try {
            if constexpr (std::is_same<T, std::string>::value) {
                value = group[settingName.c_str()].c_str();
            } else if constexpr (std::is_integral<T>::value) {
                value = group[settingName.c_str()];
            } else {
                throw std::runtime_error("Unsupported type");
            }
        } catch (const SettingNotFoundException &nfex) {
            std::cerr << "No '" << settingName << "' setting in configuration." << std::endl;
            exit(EXIT_FAILURE);
        }
        return value;
    }
};

#endif //CONFIGHELPER_H
