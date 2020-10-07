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

    cout << "server address : " << '\"' << SERVERADDRESS << '\"';
    cout << "modify to: " << flush;
    cin >> temp;
    if(!temp.empty())
        SERVERADDRESS = temp;

    cout << "\nlisten port: " << '\"' << PORT << '\"';
    cout << "modify to: " << flush;
    cin >> temp;
    if(!temp.empty()){
        istringstream is(temp);
        is >> PORT;
    }

    cout << "\nmax connection: " << '\"' << MAXCONNECTION << '\"';
    cout << "modify to: " << flush;
    cin >> temp;
    if(!temp.empty()){
        istringstream is(temp);
        is >> MAXCONNECTION;
    }

    cout << "\nbuffer length: " << '\"' << BUFFERLENGTH << '\"';
    cout << "modify to: " << flush;
    cin >> temp;
    if(!temp.empty()){
        istringstream is(temp);
        is >> BUFFERLENGTH;
    }

    cout << "\nresource root directory: " << '\"' << DIRECTORY << '\"';
    cin >> temp;
    if(!temp.empty()){
        DIRECTORY = temp;
    }
}