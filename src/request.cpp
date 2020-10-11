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

void RequestTask::parse(string header){
    if(header.empty())
        throw runtime_error("ERROR : empty header");
    get = new HttpGetHeader(header);
    res = new HttpResponseHeader(*get, Config::RESOURCE);
    stringstream(res->get_length()) >> file_length;
}

void RequestTask::prepare_file(){
    file_stream = new ifstream(Config::RESOURCE + get->get_req_file(), ios::binary);
    if(!(*file_stream)){
        file_stream = new ifstream(Config::RESOURCE + "\\404.html", ios::binary);
    }
}