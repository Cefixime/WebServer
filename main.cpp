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

    // 启动服务器
    int state = 1;
    string start;
    while(true){
        if(state == 1){
            cout << "STRART SERVEICE ?:Y/N\n";
            cin >> start;
            cin.get();
            if(start == "Y"){
                // 修改服务器配置
                Server server;
                Config::set_config();
                if(!server.WinsockStartup() && !server.ServerStartup() && !server.ListenStartup()){
                    state = server.Loop();
                    if(server.WinsockStop())
                        break;
                }
                else
                    return 1;
            } else if (start == "N")
                break;
        } else {
            cout << "SERVICE ERROR\n";
            break;
        }
    }
    return 0;
}