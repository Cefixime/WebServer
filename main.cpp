#include <winsock2.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <string.h>

#include "config.h"

using namespace std;
int main(){
    // 初始化winsock
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0) {
        return 1;
    }
    
    // 初始化服务器配置
    Config::BUFFERLENGTH = 128;
    Config::MAXCONNECTION = 5;
    Config::PORT = 80;
    Config::SERVERADDRESS = "127.0.0.1";
    Config::DIRECTORY = "C:\\MyOwn\\WorkSpace\\CPP\\sockets\\WebServer\\resource";
    // TODO:
    // 可修改服务器配置



    auto recv_buffer = new char[Config::BUFFERLENGTH];       // 接受缓冲区
    auto send_buffer = new char[Config::BUFFERLENGTH];       // 发送缓冲区


    // 配置socket
    auto listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == INVALID_SOCKET){
        cout << "Fail to create listen socket" << endl;
        WSACleanup();
        return 1;
    }
    // 绑定IP和端口
    sockaddr_in addrListen;
    addrListen.sin_family = AF_INET;                                                     //指定IP格式
    addrListen.sin_port = htons(Config::PORT);                                           //绑定端口号
    addrListen.sin_addr.S_un.S_addr = inet_addr(Config::SERVERADDRESS.c_str()); 
    if (bind(listen_socket, (SOCKADDR *)&addrListen, sizeof(addrListen)) == SOCKET_ERROR){
        cout << "bind error" << endl;
        closesocket(listen_socket);
        return 1;
    }

    // 开始监听
    if (listen(listen_socket, Config::MAXCONNECTION) == SOCKET_ERROR){
        cout << "listen error" << endl;
        closesocket(listen_socket);
        return 1;
    }

    // 等待连接
    sockaddr_in receive_addr;
    while(true){
        memset(recv_buffer, 0, Config::BUFFERLENGTH);
        memset(send_buffer, 0, Config::BUFFERLENGTH);
        auto recv_socket = accept(listen_socket, (sockaddr*)&receive_addr, new int());
        if(recv_socket == INVALID_SOCKET){
            cout << "server receive failed\n" <<"error num: " << WSAGetLastError() << endl;
            closesocket(listen_socket);
            WSACleanup();
            return 1;
        } else {
            cout << "client connection successful!" <<  endl;
        }

        // 接受报文
        ofstream recv_temp(Config::DIRECTORY + string("/") + "recv_temp.txt", ios::trunc);
        while(true){
            auto receive_data_len = recv(recv_socket, recv_buffer, Config::BUFFERLENGTH, 0);
            if(recv_temp.is_open()){
                recv_temp << recv_buffer;
            }
            if(receive_data_len == 0)
                break;
        }

        // TODO:
        // 处理报文

        // TODO:
        // 发送回复报文
        cin.get(send_buffer, Config::BUFFERLENGTH).get();
        if(send(recv_socket, send_buffer, Config::BUFFERLENGTH, 0) == SOCKET_ERROR){
            cout << "server response failed!!!" << " error num is: " << WSAGetLastError() << endl;
        }

        recv_temp.close();


        closesocket(recv_socket);
    }
    // 结束winsock
    WSACleanup();
    return 0;
}