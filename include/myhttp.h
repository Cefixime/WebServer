#ifndef MYHTTP_INC
#define MYHTTP_INC
#include <iostream>
#include <string>
#include <cstring>

class HttpFileType{
public:
    static const std::string html;      
    static const std::string ico;
    static const std::string png;
    static const std::string jpg;
    static const std::string jpeg;
    static const std::string gif;
    static const std::string mp3;
    static const std::string mp4;
};

class HttpHeader{
protected:
    std::string _header;
    std::string cons_header(std::string status, std::string file_type, std::string file_length);
    HttpHeader(std::string header):_header(std::move(header)){};
    HttpHeader() = default;
};

class HttpGetHeader:HttpHeader{
private:                           
    std::string _req_file;
public:
    std::string get_header() const{return _header;};        // 响应头
    std::string get_req_file() const{return _req_file;};   // 请求的文件
    HttpGetHeader(std::string header);              
};


class HttpResponseHeader:HttpHeader{
private:
    std::string _status;                                // 状态码
    std::string _file_length;                           // 文件长度
    std::string _file_type;                             // 文件类型                              // 响应头
public:
    // 从请求报头中解析得到响应头
    HttpResponseHeader(HttpGetHeader& get_header, std::string resource_dir);

    // 得到响应头的字符串
    std::string get_header() const {return _header;};
};

#endif