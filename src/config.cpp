#include "config.h"
#include <string>
#include <sstream>
#include <iostream>

using namespace std;


// 初始化服务器配置
int Config::BUFFERLENGTH = 128;
int Config::MAXCONNECTION = 5;
int Config::PORT = 80;
std::string Config::SERVERADDRESS = "127.0.0.1";
std::string Config::DIRECTORY = "C:\\MyOwn\\WorkSpace\\CPP\\sockets\\WebServer\\resource";

// 用户修改服务器配置
void Config::set_config(){
    string temp;

    cout << "server address " << '\"' << SERVERADDRESS << "\":" << flush;
    getline(cin, temp);
    if(!temp.empty())
        SERVERADDRESS = temp;

    cout << "listen port " << '\"' << PORT << "\":" << flush;
    getline(cin, temp);
    if(!temp.empty()){
        istringstream is(temp);
        is >> PORT;
    }

    cout << "max connection " << '\"' << MAXCONNECTION << "\":" << flush;
    getline(cin, temp);
    if(!temp.empty()){
        istringstream is(temp);
        is >> MAXCONNECTION;
    }

    cout << "buffer length " << '\"' << BUFFERLENGTH << "\":" << flush;
    getline(cin, temp);
    if(!temp.empty()){
        istringstream is(temp);
        is >> BUFFERLENGTH;
    }

    cout << "resource root directory " << '\"' << DIRECTORY << "\":" << flush;
    getline(cin, temp);
    if(!temp.empty()){
        DIRECTORY = temp;
    }
}