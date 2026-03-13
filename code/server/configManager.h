#pragma once

#include <iostream>
#include <assert.h>
#include <mutex>

#include "SimpleIni.h"

class ConfigManager {
public:
    static ConfigManager* Instance();
    void Init(const char* ConfigPath);

    template<typename T>
    T GetCfgValue(const char* Section, const char* Key, T Default);

    template<typename T>
    bool SetCfgValue(const char* Section, const char* Key, T Value);

    //////////////

    template<typename T>
    T GetValue_server(const char* Key, T Default);

    template<typename T>
    T GetValue_log(const char* Key, T Default);

    template<typename T>
    T GetValue_database(const char* Key, T Default);

    template<typename T>
    T GetValue_thread(const char* Key, T Default);

private:
    ConfigManager() :isValid_(false), isChanged_(false) {};
    ~ConfigManager() { if (isChanged_) { SaveConfig_(); } };

    void SaveConfig_();

private:
    bool isValid_;
    bool isChanged_;
    std::string configPath_;

    CSimpleIniA ini_;

    std::mutex mtx_;
};

//////////////////// 模板方法 ///////////////////////////////

template<typename T>
inline T ConfigManager::GetCfgValue(const char* Section, const char* Key, T Default) {
    std::lock_guard<std::mutex> locker(mtx_);
    if (!isValid_) { return Default; }

    if constexpr (std::is_same_v<T, const char*>) {
        return ini_.GetValue(Section, Key, Default);
    }
    else if constexpr (std::is_same_v<T, bool>) {
        return ini_.GetBoolValue(Section, Key, Default);
    }
    else if constexpr (std::is_integral_v<T>) {
        return static_cast<T>(ini_.GetLongValue(Section, Key, static_cast<long>(Default)));
    }
    else if constexpr (std::is_floating_point_v<T>) {
        return static_cast<T>(ini_.GetLongValue(Section, Key, static_cast<double>(Default)));
    }
    else {
        return Default;
    }
}

template<typename T>
inline bool ConfigManager::SetCfgValue(const char* Section, const char* Key, T Value) {
    std::lock_guard<std::mutex> locker(mtx_);
    if (!isValid_) { return false; }

    SI_Error flag = -1;
    if constexpr (std::is_same_v<T, const char*>) {
        flag = ini_.SetValue(Section, Key, Value);
    }
    else if constexpr (std::is_same_v<T, bool>) {
        flag = ini_.SetBoolValue(Section, Key, Value);
    }
    else if constexpr (std::is_integral_v<T>) {
        flag = ini_.SetLongValue(Section, Key, static_cast<long>(Value));
    }
    else if constexpr (std::is_floating_point_v<T>) {
        flag = ini_.GetLongValue(Section, Key, static_cast<double>(Value));
    }
    else {
        flag = -1;
    }

    bool ret = (flag == SI_OK || flag == SI_UPDATED);
    if (!isChanged_ && ret) isChanged_ = true;
    return ret;
}

template<typename T>
inline T ConfigManager::GetValue_server(const char* Key, T Default) {
    return GetCfgValue("server", Key, Default);
}

template<typename T>
inline T ConfigManager::GetValue_log(const char* Key, T Default) {
    return GetCfgValue("log", Key, Default);
}

template<typename T>
inline T ConfigManager::GetValue_database(const char* Key, T Default) {
    return GetCfgValue("database", Key, Default);
}

template<typename T>
inline T ConfigManager::GetValue_thread(const char* Key, T Default) {
    return GetCfgValue("thread", Key, Default);
}
