#include "config.h"
#include <string>
#include <sstream>
#include <iostream>

using namespace std;


// 初始化服务器配置
int Config::BUFFERLENGTH = 1024;
int Config::MAXCONNECTION = 20;
int Config::PORT = 80;
std::string Config::SERVERADDRESS = "127.0.0.1";
std::string Config::RESOURCE = "C:\\MyOwn\\WorkSpace\\CPP\\sockets\\WebServer\\resource";
std::string Config::CACHE = "C:\\MyOwn\\WorkSpace\\CPP\\sockets\\WebServer\\cache";
// 用户修改服务器配置
void Config::set_config(){
    string temp;

    cout << "default server address " << '\"' << SERVERADDRESS << "\":" << flush;
    getline(cin, temp);
    if(!temp.empty())
        SERVERADDRESS = temp;

    cout << "default listen port " << '\"' << PORT << "\":" << flush;
    getline(cin, temp);
    if(!temp.empty()){
        istringstream is(temp);
        is >> PORT;
    }

    cout << "default max connection " << '\"' << MAXCONNECTION << "\":" << flush;
    getline(cin, temp);
    if(!temp.empty()){
        istringstream is(temp);
        is >> MAXCONNECTION;
    }

    cout << "default buffer length " << '\"' << BUFFERLENGTH << "\":" << flush;
    getline(cin, temp);
    if(!temp.empty()){
        istringstream is(temp);
        is >> BUFFERLENGTH;
    }

    cout << "default resource root directory " << '\"' << RESOURCE << "\":" << flush;
    getline(cin, temp);
    if(!temp.empty()){
        RESOURCE = temp;
    }

    cout << "default cache root directory " << '\"' << CACHE << "\":" << flush;
    getline(cin, temp);
    if(!temp.empty()){
        CACHE = temp;
    }
}