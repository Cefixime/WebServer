#include <fstream>
#include <string>
#include <sstream>
#include "myhttp.h"
#define MAX_LINE_LENGTH 128
#define ERROR_FILE_LEN 138      // 404 not found 页面的长度
using namespace std;

const string HttpFileType::html = string("text/html");
const string HttpFileType::ico = string("image/x-icon");
const string HttpFileType::png = string("text/html");
const string HttpFileType::jpg = string("image/jpeg");
const string HttpFileType::jpeg = string("image/jpeg");
const string HttpFileType::gif = string("image/gif");
const string HttpFileType::mp3 = string("audio/mp3");
const string HttpFileType::mp4 = string("video/mpeg4");

string HttpHeader::cons_header(string status, string file_type, string file_length){
    return  "HTTP/1.1 " + status +"\r\n" +                        \
            "Content-Type: " + file_type + ";charset=UTF-8\r\n"   \
            "Content-Length: " + file_length + "\r\n\r\n";
}

HttpGetHeader::HttpGetHeader(string header)
    :HttpHeader(header){
    istringstream istring(_header);
    // 只要一行
    auto temp = new char[MAX_LINE_LENGTH];
    istring.getline(temp, MAX_LINE_LENGTH);
    string first_line_header(temp);
    delete[] temp;

    string req_file;
    istringstream ifirst_line(first_line_header);
    ifirst_line >> req_file;        // GET
    ifirst_line >> req_file;        // <file_path>
    _req_file = req_file;
}

HttpResponseHeader::HttpResponseHeader(HttpGetHeader& get_header, string resource_dir){
    ifstream fin(get_header.get_req_file());
    if(fin){
        // 状态码
        _status = string("200 OK"); 

        // 文件类型
        auto req_file = get_header.get_req_file();
        auto start_pos = req_file.find('.');
        string file_type = req_file.substr(start_pos + 1);
        if(file_type == "html"){
            _file_type = HttpFileType::html;
        } else if(file_type == "ico"){
            _file_type = HttpFileType::ico;
        } else if (file_type == "jpg"){
            _file_type = HttpFileType::jpg;
        } else if (file_type == "png"){
            _file_type = HttpFileType::png;
        } else if (file_type == "jpeg"){
            _file_type = HttpFileType::jpeg;
        } else if (file_type == "gif"){
            _file_type = HttpFileType::gif;
        } else if (file_type == "mp3"){
            _file_type = HttpFileType::mp3;
        } else if (file_type == "mp4"){
            _file_type = HttpFileType::mp4;
        }

        // 文件长度
        fin.seekg(0, ios::end);
        _file_length = to_string(fin.tellg());
        fin.seekg(0, ios::beg);

        // 构建响应头
    } else {
        _status = string("404 Not Found");
        _file_type = HttpFileType::html;
        _file_length = to_string(ERROR_FILE_LEN);

    }
    _header = cons_header(_status, _file_type, _file_length);
}