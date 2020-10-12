#include <iostream>
#include <string>
#include <algorithm>
#include <cstdio>
#include <thread>
#include <mutex>
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
        cout << "ERROR : Winsock start fail" << endl;
        return -1;
    }
    return 0;
}

int Server::ServerStartup(){
    // 配置socket
    srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_socket == INVALID_SOCKET){
        cout << "ERROR : Fail to create listen socket" << endl;
        return -1;
    }
    // 绑定IP和端口
    srv_addr.sin_family = AF_INET;                                                     //指定IP格式
    srv_addr.sin_port = htons(Config::PORT);                                           //绑定端口号
    srv_addr.sin_addr.S_un.S_addr = inet_addr(Config::SERVERADDRESS.c_str());          // 地址
    if (bind(srv_socket, (SOCKADDR *)&srv_addr, sizeof(srv_addr)) == SOCKET_ERROR){
        cout << "ERROR : bind error" << endl;
        closesocket(srv_socket);
        return -1;
    }
    return 0;
}

int Server::ListenStartup(){
    if (listen(srv_socket, Config::MAXCONNECTION) == SOCKET_ERROR){
        cout << "ERROR : listen error" << endl;
        closesocket(srv_socket);
        return -1;
    } else
        return 0;
}

int Server::WinsockStop(){
    return WSACleanup();
}

int Server::Loop(){
    fd_set rdfs;
    fd_set wdfs;
    FD_ZERO(&wdfs);
    sockaddr_in client_addr;

    int client_addrlen = sizeof(client_addr);

    while (true) {
        auto new_sess = accept(srv_socket, (LPSOCKADDR)&client_addr, &client_addrlen);
        if(new_sess != INVALID_SOCKET){
            io_lock.lock();
            cout << "INFO : session connect : " << '\"' << get_addr(new_sess) + ":" << get_port(new_sess) <<" socket:" <<new_sess << "\"\n" << flush;
            io_lock.unlock();
            thread t(Server::session_handler, this, new_sess);
            t.detach();
        }
    }
    cout << "over" << endl;
    return 0;
}

void Server::session_handler(Server* srv, SOCKET new_sess, bool* is_alive){
        auto reqtask = RequestTask();
        reqtask.parse(srv->recv_mes(new_sess));
        reqtask.prepare_file();

        reqtask.state = RequestState::WAITING_RESPONSE;
        while(true){
            if(reqtask.state == RequestState::WAITING_RESPONSE || reqtask.state == RequestState::WAITING_FILE){
                srv->send_mes(new_sess, reqtask);
            }
            if(reqtask.state == RequestState::FINISH) break;
        }
        srv->state_lock.lock();
        *is_alive = false;
        srv->state_lock.unlock();
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

string Server::recv_mes(SOCKET s){
    this->recv_lock.lock();
    memset(recv_buf, 0, Config::BUFFERLENGTH);
    stringstream recv_stream;
    int bytes_num = recv(s, recv_buf, Config::BUFFERLENGTH, 0);
    if(bytes_num == 0 | bytes_num == SOCKET_ERROR){
        closesocket(s);
        io_lock.lock();
        cout << "WARNING : invalid socket:" << s << '\n' << flush;
        io_lock.unlock();
    } else {
        io_lock.lock();
        cout << "\nINFO : HTTP request information from socket:" << s << "\n" << flush;
        recv_stream.write(recv_buf, bytes_num);
        cout << recv_stream.str() << endl;
        io_lock.unlock();
    }
    this->recv_lock.unlock();
    return recv_stream.str();
}

bool Server::sess_finished(bool* sess){
    return ~(*sess);
}

void Server::remove_sess(){
    auto it_b = remove_if(alive_threads.begin(), alive_threads.end(), Server::sess_finished);
    for(auto it = it_b; it != alive_threads.end();it++){
        delete *it;
    }
    alive_threads.erase(it_b, alive_threads.end());
}

void Server::send_mes(SOCKET s, RequestTask& rt){
    this->send_lock.lock();
    memset(send_buf, 0, Config::BUFFERLENGTH);
    int read_size;
    if(rt.state == RequestState::WAITING_RESPONSE){
        read_size = rt.res->get_header().length();
        auto sin = stringstream(rt.res->get_header());
        sin.read(send_buf, read_size);
    } else if(rt.state == RequestState::WAITING_FILE) {
        read_size = Config::BUFFERLENGTH > rt.file_length - rt.offset ? rt.file_length - rt.offset : Config::BUFFERLENGTH;
        rt.file_stream->read(send_buf, read_size);
    } else throw runtime_error("ERROR : wrong state of request task");

    int bytes_num = send(s, send_buf, read_size, 0);
    this->send_lock.unlock();
    if( bytes_num == SOCKET_ERROR | bytes_num == 0){
        closesocket(s);
        io_lock.lock();
        cout << "ERROR :  send error" << '\n';
        cout << "ERROR num: " << WSAGetLastError() << '\n';
        io_lock.unlock();
        rt.state = RequestState::FINISH;
        rt.file_stream->close();
    } else {
        if(rt.state == RequestState::WAITING_RESPONSE){
            rt.state = RequestState::WAITING_FILE;
        } else {
            rt.offset += read_size;
        }
    }
    if(rt.offset == rt.file_length){
        rt.state = RequestState::FINISH;
        rt.file_stream->close();
        io_lock.lock();
        cout << "INFO : finish send:" << rt.get->get_req_file() << "\n\n";
        cout << "WARNING : close socket" << s << '\n' << flush;
        io_lock.unlock();
        closesocket(s);
    }
}