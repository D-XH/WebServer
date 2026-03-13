#include "configManager.h"

#include<iostream>
ConfigManager* ConfigManager::Instance() {
    static ConfigManager ins;
    return &ins;
}

void ConfigManager::Init(const char* ConfigPath) {
    std::cout << ConfigPath << std::endl;
    SI_Error rc = ini_.LoadFile(ConfigPath);
    if (rc < 0) {
        std::cerr << "ConfigManager init err! " << rc << std::endl;
        std::abort();
    }
    configPath_ = std::string(ConfigPath);
    isValid_ = true;
}

void ConfigManager::SaveConfig_() {
    SI_Error rc = ini_.SaveFile(configPath_.c_str());
    if (rc < 0) {
        std::cerr << "ConfigManager save err! " << rc << std::endl;
        std::abort();
    }
}