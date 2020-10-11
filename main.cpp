#include <winsock2.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <string.h>
#include "server.h"
#include "config.h"

using namespace std;
int main(){

    Server server;
    // 启动服务器
    if(server.WinsockStartup() != 0) return 1;
    if(server.ServerStartup() != 0) return 1;

    // 修改服务器配置
    Config::set_config();

    if(server.ListenStartup() != 0) return 1;
    if(server.Loop() != 0) return 1;
    if(server.WinsockStop() != 0) return 1;
    return 0;
}