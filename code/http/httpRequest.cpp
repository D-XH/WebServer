#include "httpRequest.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{ "/index", "/register", "/login", "/welcome", "/video", "/picture" };
const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{ {"/register.html", 0}, {"/login.html", 1} };

HttpRequest::HttpRequest() {
    Init();
}

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

// 解析http请求
bool HttpRequest::Parse(Buffer& Buff) {
    const char CRLF[] = "\r\n";
    if (Buff.ReadableBytes() <= 0) {
        LOG_WARN("缓存为空！无法解析。");
        return false;
    }

    // 逐行解析
    while (Buff.ReadableBytes() && state_ != ParseState::FINISH) {
        const char* lineEnd = std::search(Buff.Peek(), Buff.Peek() + Buff.ReadableBytes(), CRLF, CRLF + 2); // 找到行尾
        std::string line(Buff.Peek(), lineEnd); // 取出一行
        Buff.RetrieveUntil(lineEnd + 2);

        StateChange_(line);


        if (Buff.ReadableBytes() <= 2) {
            state_ = ParseState::FINISH;
        }
    }
    return true;
}

std::string& HttpRequest::GetPath() {
    return path_;
}

bool HttpRequest::IsKeepAlive() {
    if (header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

// 状态机
void HttpRequest::StateChange_(const std::string& Line) {
    switch (state_) {
    case ParseState::REQUEST_LINE:
        ParseRequestLine_(Line) && ParsePath_();
        break;
    case ParseState::HEADER:
        ParseHeader_(Line);
        break;
    case ParseState::BODY:
        ParseBody_(Line);
        break;
    case ParseState::FINISH:
        break;
    default:
        break;
    }
}

// 解析请求行
bool HttpRequest::ParseRequestLine_(const std::string& Line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;

    if (std::regex_match(Line, subMatch, patten)) {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];

        state_ = ParseState::HEADER;
        return true;
    }
    LOG_ERROR("Parse Request Line failed!");
    return false;
}

// 解析请求头
void HttpRequest::ParseHeader_(const std::string& Line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(Line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    }
    else {
        state_ = ParseState::BODY;
    }
}

// 解析请求体
void HttpRequest::ParseBody_(const std::string& Line) {
    body_ = Line;
    ParsePost_();
    state_ = ParseState::FINISH;
    LOG_DEBUG("Body:%s, len:%d", Line.c_str(), Line.size());
}

// 解析url
bool HttpRequest::ParsePath_() {
    if (path_ == "/") {
        path_ = "/index.html";
    }
    else {
        for (auto&& item : DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
    return true;
}

// 解析
void HttpRequest::ParsePost_() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            // 登录或者注册
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag: %d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = tag;
                if (UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                }
                else {
                    path_ = "error.html";
                }
            }
        }
    }
}

// 解码
void HttpRequest::ParseFromUrlencoded_() {
    if (body_.size() == 0) { return; }

    std::string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for (; i < n; ++i) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) + 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key, value);
            break;
        default:
            break;
        }
    }

    assert(j <= i);
    if (post_.count(key) == 0 && j <= i) {
        // 处理最后一个
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

// 把十六进制字母，转为十进制
int HttpRequest::ConverHex(char ch) {
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
}

// 登录或注册
bool HttpRequest::UserVerify(const std::string& UserName, const std::string& Pwd, bool IsLogin) {
    if (UserName == "" || Pwd == "") { return false; }

    SqlConnRAII connRAII;
    MYSQL* conn = connRAII.GetConn();
    char sql[256] = { 0 };
    snprintf(sql, 256, "select UserName, PassWord from tbl_User where UserName='%s' and del=0;", UserName.c_str());

    if (mysql_query(conn, sql)) {
        LOG_ERROR("UserVerify failed! mysql query failed. [file: %s][line: %d]", __FILE__, __LINE__);
        return false;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (MYSQL_ROW row = mysql_fetch_row(res)) {
        if (!IsLogin) {
            mysql_free_result(res);
            LOG_DEBUG("Register UserName Repeat!");
            return false;
        }

        std::string pwd(row[1]);
        mysql_free_result(res);
        return pwd == Pwd;
    }
    else {
        if (IsLogin) {
            mysql_free_result(res);
            LOG_DEBUG("Login UserName error!");
            return false;
        }

        mysql_free_result(res);
        memset(sql, 0, 256);
        snprintf(sql, 256, "insert into tbl_User(UserName, PassWord) values('%s', '%s');", UserName.c_str(), Pwd.c_str());

        if (mysql_query(conn, sql)) {
            LOG_ERROR("UserVerify failed! mysql query failed. [file: %s][line: %d]", __FILE__, __LINE__);
            return false;
        }
    }

    LOG_DEBUG("UserVerify Success!");
    return true;
}
