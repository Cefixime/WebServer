#include <iostream>
#include <string>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <winsock2.h>
#include "config.h"
#include "server.h"
#include "request.h"
using namespace std;

Server::Server(){
    recv_buf = new char[Config::BUFFERLENGTH];
    send_buf = new char[Config::BUFFERLENGTH];
    srv_socket = INVALID_SOCKET;
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
    srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_socket == INVALID_SOCKET){
        cout << "Fail to create listen socket" << endl;
        return -1;
    }
    // 绑定IP和端口
    srv_addr.sin_family = AF_INET;                                                     //指定IP格式
    srv_addr.sin_port = htons(Config::PORT);                                           //绑定端口号
    srv_addr.sin_addr.S_un.S_addr = inet_addr(Config::SERVERADDRESS.c_str());          // 地址
    if (bind(srv_socket, (SOCKADDR *)&srv_addr, sizeof(srv_addr)) == SOCKET_ERROR){
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
    sockaddr_in client_addr;
    int client_addrlen = sizeof(client_addr);
    //非阻塞模式
    if(ioctlsocket(srv_socket, FIONBIO, block_mode) == SOCKET_ERROR) {     
        cout << "ioctlsocket() for new session failed with error!\n";
        return -1;
    }
    while (true) {
        // 设置socket集合
        FD_ZERO(rfds);
        FD_ZERO(wfds);
        FD_SET(srv_socket, rfds);

        remove_invalid_sockets();

        for(auto &sess: sess_sockets){
            FD_SET(sess, rfds);
            FD_SET(sess, wfds);
        }

        // 接受信号
        socket_signal = select(0, rfds, wfds, NULL, NULL);

        if(socket_signal == SOCKET_ERROR){
            cout << "select error!" << endl;
            cout << WSAGetLastError();
            return -1;
        }

        if(FD_ISSET(srv_socket, rfds)){
            socket_signal--;
            
            // 接受连接
            auto new_sess = accept(srv_socket, (LPSOCKADDR)&client_addr, &client_addrlen);

            if(new_sess != INVALID_SOCKET){
                sess_sockets.push_back(new_sess);
                cout << "session connect : " << '\"' << get_addr(new_sess) + ":" << get_port(new_sess) <<" socket:" <<new_sess << "\"\n";
                // 设置非阻塞
                if(ioctlsocket(new_sess, FIONBIO, block_mode) == SOCKET_ERROR){
                    cout << "ioctlsocket() for new session failed with error!\n";
                    return -1;
                }
            }
        }

        if(socket_signal > 0){
            for(auto sess:sess_sockets){
                // 会话socket有信号产生
                if(FD_ISSET(sess, rfds)){
                    if(req_map.find(sess) == req_map.end())
                        req_map[sess] = RequestTask();
                    cout << "--message from client\n";
                    socket_signal--;
                    recv_mes(sess, Config::CACHE + "\\recv_temp" + to_string(sess) + ".txt");
                    auto &req = req_map[sess];
                    if(req.state == RequestState::UNSOLVE ) req.state = RequestState::IN_STARTING;
                } else if(req_map[sess].state == RequestState::IN_STARTING){
                    req_map[sess].state = RequestState::WAITING_RESPONSE;
                    // TODO:解析报头
                    auto parse_result = parse(sess);
                    // remove((Config::CACHE + "\\recv_temp" + to_string(sess)).c_str());  // 删除报头文件
                    req_map[sess].file_path = parse_result.first;
                    req_map[sess].file_type = parse_result.second;
                    //TODO:准备文件
                    prepare_file(req_map[sess]);
                }

                if(FD_ISSET(sess, wfds)){
                    socket_signal--;
                    if(req_map[sess].state == RequestState::WAITING_RESPONSE){
                        cout << "--send to client";
                        // TODO:根据解析内容回复信息
                        send_mes(sess, req_map[sess].file_stream, req_map[sess].state);
                    }
                }
                // 服务器socket有信号产生
            }
        }
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
    memset(recv_buf, 0, Config::BUFFERLENGTH);
    stringstream recv_stream;
    while(true){
        int bytes_num = recv(s, recv_buf, Config::BUFFERLENGTH, 0);
        if(bytes_num == 0 | bytes_num == SOCKET_ERROR){
            closesocket(s);
            break;
        } else {
            recv_stream << "message part"<< part <<":\n";
            recv_stream << recv_buf;
            recv_stream << "\nmessage part "<< part << " end\n";
            part++;
        }
    }
    cout << recv_stream.str() << endl;
}

void Server::recv_mes(SOCKET s, string file_path){
    memset(recv_buf, 0, Config::BUFFERLENGTH);
    stringstream recv_stream;
    fstream outfile;
    int bytes_num = recv(s, recv_buf, Config::BUFFERLENGTH - 1, 0);
    if(bytes_num == 0 | bytes_num == SOCKET_ERROR){
        closesocket(s);
        cout << "--invlida socket" << s << '\n';
        invalid_sockets.push_back(s);
    } else {
        outfile.open(file_path, ios::app|ios::out|ios::binary);
        cout << "--getdata" <<" socket:"<< s << endl;
        outfile << string(recv_buf) << flush;
        outfile.close();
    }
}

void Server::remove_invalid_sockets(){
    for(auto& s: invalid_sockets){
        sess_sockets.remove(s);
    }
}

void Server::send_mes(SOCKET s, ifstream* rfile, RequestState& finished){
    memset(send_buf, 0, Config::BUFFERLENGTH);
    rfile->read(send_buf, Config::BUFFERLENGTH);
    if(rfile->cur == rfile->end && finished != RequestState::FINISH)
        finished = RequestState::FINISH;
    int bytes_num = send(s, send_buf, Config::BUFFERLENGTH, 0);
    if( bytes_num == SOCKET_ERROR | bytes_num == 0){
        cout << "send error" << endl;
    } else {
        cout << "send " << bytes_num << " bytes\n";
        cout << send_buf;
    }
}