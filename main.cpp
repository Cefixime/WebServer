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

    auto server = Server();
    // 启动服务器
    if(server.WinsockStartup() != 0) return 1;
    if(server.ServerStartup() != 0) return 1;

    // 修改服务器配置
    Config::set_config();

    if(server.ListenStartup() != 0) return 1;
    if(server.Loop() != 0) return 1;
    if(server.WinsockStop() != 0) return 1;
    // 开始运行
    
    // while(true){
    //     memset(recv_buffer, 0, Config::BUFFERLENGTH);
    //     memset(send_buffer, 0, Config::BUFFERLENGTH);
    //     auto recv_socket = accept(listen_socket, (SOCKADDR*)&receive_addr, new int(sizeof(receive_addr)));
    //     if(recv_socket == INVALID_SOCKET){
    //         cout << "server receive failed\n" <<"error num: " << WSAGetLastError() << endl;
    //         closesocket(listen_socket);
    //         WSACleanup();
    //         return 1;
    //     } else {
    //         cout << "client connection successful!" <<  endl;
    //     }

    //     // 接受报文
    //     ofstream recv_temp(Config::DIRECTORY + string("/") + "recv_temp.txt", ios::trunc);
    //     int i = 0;
    //     while(true){
    //         auto receive_data_len = recv(recv_socket, recv_buffer, Config::BUFFERLENGTH, 0);
    //         if(recv_temp.is_open() && receive_data_len > 0){
    //             recv_temp << recv_buffer;
    //         } else
    //             break;
    //     }
    //     recv_temp.close();

    //     ifstream in(Config::DIRECTORY + string("/") + "recv_temp.txt", ios::in);
    //     for(string str; in >> str;)
    //         cout<<str<<endl;

    //     // TODO:
    //     // 处理报文

    //     // TODO:
    //     // 发送回复报文
    //     cin.get(send_buffer, Config::BUFFERLENGTH).get();
    //     if(send(recv_socket, send_buffer, Config::BUFFERLENGTH, 0) == SOCKET_ERROR){
    //         cout << "server response failed!!!" << " error num is: " << WSAGetLastError() << endl;
    //     }



    //     closesocket(recv_socket);
    // }
    // 结束winsock
    // WSACleanup();
    return 0;
}