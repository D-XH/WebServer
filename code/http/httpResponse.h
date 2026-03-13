#pragma once

#include <unordered_map>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& SrcDir, std::string& Path, bool IsKeepAlive, int Code);
    void MakeResponse(Buffer& Buff);

    void UnmapFile();
    size_t GetFileSize();
    char* GetFileMMPtr();
private:
    void AddStateLine_(Buffer& Buff);
    void AddHeader_(Buffer& Buff);
    void AddContent_(Buffer& Buff);

    std::string GetFileType_();
    void ErrorHtml_();
    void ErrorContent_(Buffer& Buff, std::string msg);
private:
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;  // 后缀类型集
    static const std::unordered_map<int, std::string> CODE_STATUS;          // 编码状态集
    static const std::unordered_map<int, std::string> CODE_PATH;            // 编码路径集
private:
    int code_;

    struct stat respFileStat_;
    char* respFileMMPtr_;

    bool isKeepAlive_;
    std::string srcDir_, path_;
};
