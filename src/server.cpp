#include <iostream>
#include <string>
#include <fstream>
#include <winsock2.h>
#include "config.h"
#include "server.h"

using namespace std;

Server::Server(){
    recv_buf = new char[Config::BUFFERLENGTH];
    send_buf = new char[Config::BUFFERLENGTH];
    srv_socket = INVALID_SOCKET;
    session_socket = INVALID_SOCKET;
    rfds = new fd_set();
    wfds = new fd_set();
    block_mode = new u_long(UNBLOCKING);
    ready_send = false;
}

Server::~Server(){
    if(recv_buf != nullptr)
        delete[] recv_buf;
    if(send_buf != nullptr)
        delete[] send_buf;
    if(srv_socket != INVALID_SOCKET)
        srv_socket = INVALID_SOCKET;
}

int Server::WinsockStartup(){
    auto version = MAKEWORD(2,2);
    auto wsadata = new WSADATA();
    auto wsa_result = WSAStartup(version, (LPWSADATA)wsadata);
    if(wsa_result != 0){
        cout << "Winsock start fail" << endl;
        return -1;
    }
    return 0;
}

int Server::ServerStartup(){
    // 配置socket
    auto srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_socket == INVALID_SOCKET){
        cout << "Fail to create listen socket" << endl;
        return -1;
    }
    // 绑定IP和端口
    sockaddr_in addrListen;
    addrListen.sin_family = AF_INET;                                                     //指定IP格式
    addrListen.sin_port = htons(Config::PORT);                                           //绑定端口号
    addrListen.sin_addr.S_un.S_addr = inet_addr(Config::SERVERADDRESS.c_str());          // 地址
    if (bind(srv_socket, (SOCKADDR *)&addrListen, sizeof(addrListen)) == SOCKET_ERROR){
        cout << "bind error" << endl;
        closesocket(srv_socket);
        return -1;
    }
    return 0;
}

int Server::ListenStartup(){
    if (listen(srv_socket, Config::MAXCONNECTION) == SOCKET_ERROR){
        cout << "listen error" << endl;
        closesocket(srv_socket);
        return -1;
    } else
        return 0;
}

int Server::WinsockStop(){
    return WSACleanup();
}

int Server::Loop(){
    //非阻塞模式
    if(ioctlsocket(srv_socket, FIONBIO, block_mode) == SOCKET_ERROR) {     
        cout << "ioctlsocket() for new session failed with error!\n";
        return -1;
    }
    auto client_addr = new sockaddr_in();
    auto client_addrlen = new int(sizeof(*client_addr));
    while (true) {
        // 设置socket集合
        FD_ZERO(rfds);
        FD_ZERO(wfds);
        FD_SET(srv_socket, rfds);

        if(session_socket != INVALID_SOCKET){
            FD_SET(session_socket, rfds);
            //FD_SET(session_socket, wfds);
        }

        // 接受信号
        socket_signal = select(0, rfds, wfds, NULL, NULL);


        if(socket_signal == SOCKET_ERROR){
            cout << "select error!" << endl;
        }
        // 服务器socket有信号产生
        if(FD_ISSET(srv_socket, rfds)){
            socket_signal--;

            // 接受连接
            session_socket = accept(srv_socket, (LPSOCKADDR)client_addr, client_addrlen);

            if(session_socket = INVALID_SOCKET){
                if(WSAGetLastError() != WSAEWOULDBLOCK){
                    // DEBUG:解析错误码
                    cout << "Server accept connection request error!\n";
				    return -1;
                }
            } else {
                cout << "session connect : " << '\"' << get_addr(session_socket) + ":" << get_port(session_socket) << "\"\n";
                // 设置非阻塞
                if(ioctlsocket(session_socket, FIONBIO, block_mode) == SOCKET_ERROR){
                    cout << "ioctlsocket() for new session failed with error!\n";
                    return -1;
                }
            }
        }
        // 会话socket有信号产生
        if(FD_ISSET(session_socket, rfds)){
            socket_signal--;
            recv_mes(session_socket);
            // TODO:解析报头
        }

        // if(FD_ISSET(session_socket, wfds)){
        //     socket_signal--;
        //     if(ready_send){
        //         // TODO:根据解析内容回复信息
        //         send_mes(session_socket, req.file);
        //     }
        // }
    }
}

string Server::get_addr(SOCKET s){
    string string_addr;
	sockaddr_in addr;
	int nameLen,rtn;

	nameLen = sizeof(addr);
	rtn = getsockname(s,(LPSOCKADDR)&addr,&nameLen);
	if(rtn != SOCKET_ERROR){
		string_addr += inet_ntoa(addr.sin_addr);
	}
	return string_addr; 
}

int Server::get_port(SOCKET s){
	sockaddr_in addr;
	int nameLen,rtn;

	nameLen = sizeof(addr);
	rtn = getsockname(s,(LPSOCKADDR)&addr,&nameLen);
	if(rtn != SOCKET_ERROR){
		return addr.sin_port;
	}
	return -1; 
}

void Server::recv_mes(SOCKET s){
    int part = 0;
    while(true){
        int bytes_num = recv(session_socket, recv_buf, Config::BUFFERLENGTH, 0);
        if(bytes_num == SOCKET_ERROR){
            if(WSAGetLastError() != WSAEWOULDBLOCK){
                cout << "server receive error!!!\n" << flush;
                break;
            }
        } else {
            cout << "message part"<< part <<":\n";
            cout << recv_buf << '\n';
            cout << "message part"<< part << "end.\n" << flush;
            part++;
        }
    }
}