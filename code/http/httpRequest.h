#pragma once

#include <string>
#include <regex>
#include <unordered_map>
#include <unordered_set>

#include "../log/log.h"
#include "../buffer/buffer.h"
#include "../pool/sqlConnRAII.h"

class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest() = default;

    void Init();
    bool Parse(Buffer& Buff);

    std::string& GetPath();
    bool IsKeepAlive();

private:
    void StateChange_(const std::string& Line);

    bool ParseRequestLine_(const std::string& Line);   // 解析请求行
    void ParseHeader_(const std::string& Line);         // 解析请求头
    void ParseBody_(const std::string& Line);           // 解析请求体

    bool ParsePath_();
    void ParsePost_();
    void ParseFromUrlencoded_();

    int ConverHex(char ch); // 把十六进制转为十进制

    bool UserVerify(const std::string& UserName, const std::string& Pwd, bool IsLogin);

public:
    enum ParseState {
        REQUEST_LINE,
        HEADER,
        BODY,
        FINISH
    };
private:
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
private:
    ParseState state_;

    std::string method_, path_, version_, body_;    //
    std::unordered_map<std::string, std::string> header_;   // 请求头kv
    std::unordered_map<std::string, std::string> post_;     // post
    std::unordered_map<std::string, std::string> get_;     // get
};

