#include <string>
#include <fstream>
#include <sstream>
#include <winsock2.h>
#include "request.h"
#include "myhttp.h"
#include "server.h"
#include "config.h"

using namespace std;

RequestTask::~RequestTask(){
    if(file_stream != nullptr)
        file_stream->close();
    delete res;
    delete get;
}

void RequestTask::parse(SOCKET s){
    auto getheader_file = Server::cache_file(s);
    ifstream fin(getheader_file);
    if(fin){
        stringstream sin;
        sin << fin.rdbuf();
        get = new HttpGetHeader(sin.str());
        res = new HttpResponseHeader(*get, Config::RESOURCE);
        stringstream(res->get_length()) >> file_length;
    } else {
        throw runtime_error("no that cache file: " + getheader_file);
    }
}

void RequestTask::prepare_file(){
    file_stream = new ifstream(Config::RESOURCE + get->get_req_file(), ios::binary);
    if(!(*file_stream)){
        file_stream = new ifstream(Config::RESOURCE + "\\404.html", ios::binary);
    }
}