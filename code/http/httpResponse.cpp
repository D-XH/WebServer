#include "httpResponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() {
    code_ = -1;
    srcDir_ = path_ = "";
    isKeepAlive_ = false;
    respFileMMPtr_ = nullptr;
    respFileStat_ = { 0 };
}

HttpResponse::~HttpResponse() {
    UnmapFile();
}

void HttpResponse::Init(const std::string& SrcDir, std::string& Path, bool IsKeepAlive, int Code) {
    assert(SrcDir != "");

    srcDir_ = SrcDir;
    path_ = Path;
    code_ = Code;
    isKeepAlive_ = IsKeepAlive;

    if (respFileMMPtr_) { UnmapFile(); }
    respFileMMPtr_ = nullptr;
    respFileStat_ = { 0 };
}

void HttpResponse::MakeResponse(Buffer& Buff) {
    if (stat((srcDir_ + path_).data(), &respFileStat_) < 0 || S_ISDIR(respFileStat_.st_mode)) {
        code_ = 404;
    }
    else if (!(respFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if (code_ == -1) {
        code_ = 200;
    }

    ErrorHtml_();
    AddStateLine_(Buff);
    AddHeader_(Buff);
    AddContent_(Buff);
}

void HttpResponse::UnmapFile() {
    if (respFileMMPtr_) {
        munmap(respFileMMPtr_, respFileStat_.st_size);
        respFileMMPtr_ = nullptr;
    }
}

size_t HttpResponse::GetFileSize() {
    return respFileStat_.st_size;
}

char* HttpResponse::GetFileMMPtr() {
    return respFileMMPtr_;
}

void HttpResponse::AddStateLine_(Buffer& Buff) {
    std::string status;
    if (CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    Buff.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buffer& Buff) {
    // 连接控制
    if (isKeepAlive_) {
        Buff.Append("Connection: keep-alive\r\n");
        Buff.Append("keep-alive: max=6, timeout=120\r\n");
    }
    else {
        Buff.Append("Connection: close\r\n");
    }

    // 内容类型
    Buff.Append("Content-Type: " + GetFileType_() + "\r\n");

    // 文件长度
    Buff.Append("Content-length: " + std::to_string(respFileStat_.st_size) + "\r\n\r\n");
}

void HttpResponse::AddContent_(Buffer& Buff) {
    int respFd = open((srcDir_ + path_).c_str(), O_RDONLY);
    if (respFd < 0) {
        ErrorContent_(Buff, "File Not Found!");
        LOG_ERROR("AddContent failed, open file failed. [file:%s]", __FILE__);
        return;
    }

    void* mmPtr = mmap(nullptr, respFileStat_.st_size, PROT_READ, MAP_PRIVATE, respFd, 0);
    if (mmPtr == MAP_FAILED) {
        ErrorContent_(Buff, "File Not Found!");
        LOG_ERROR("AddContent failed, mmap failed. [file:%s][line:%d]", __FILE__, __LINE__);
        return;
    }

    respFileMMPtr_ = static_cast<char*>(mmPtr);
    close(respFd);
}

std::string HttpResponse::GetFileType_() {
    size_t pos = path_.find_last_of('.');
    if (pos == std::string::npos) {
        return "text/plain";
    }

    std::string suffix = path_.substr(pos);
    if (SUFFIX_TYPE.count(suffix) == 0) {
        return "text/plain";
    }
    return SUFFIX_TYPE.find(suffix)->second;
}

void HttpResponse::ErrorHtml_() {
    if (CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir_ + path_).data(), &respFileStat_);
    }
}

void HttpResponse::ErrorContent_(Buffer& Buff, std::string msg) {
    std::string body, status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        status = "Bad Request";
    }
    body += std::to_string(code_) + " : " + status + "\n";
    body += "<p>" + msg + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    Buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    Buff.Append(body);
}
