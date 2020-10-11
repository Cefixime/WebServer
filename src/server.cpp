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
    sockaddr_in client_addr;
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10;
    int client_addrlen = sizeof(client_addr);
    u_long unblock = 1;
    ioctlsocket(srv_socket, FIONBIO, &unblock);
    int signal;
    while (true) {
        FD_ZERO(&rdfs);
        FD_SET(srv_socket, &rdfs);
        signal = select(0, &rdfs, nullptr, nullptr, &tv);
        if(signal > 0 && sess_threads.size() < Config::MAXCONNECTION){
            auto new_sess = accept(srv_socket, (LPSOCKADDR)&client_addr, &client_addrlen);
            if(new_sess != INVALID_SOCKET){
                cout << "INFO : session connect : " << '\"' << get_addr(new_sess) + ":" << get_port(new_sess) <<" socket:" <<new_sess << "\"\n";
                thread t(Server::session_handler, this, new_sess);
                sess_threads.push_back(&t);
                t.detach();
            }
        }
        remove_sess();
    }
    remove_sess();
    while(!sess_threads.empty());
    return 0;
}

void Server::session_handler(Server* srv, SOCKET new_sess){
        auto reqtask = RequestTask();
        srv->recv_lock.lock();
        reqtask.parse(srv->recv_mes(new_sess));
        srv->recv_lock.unlock();
        reqtask.prepare_file();
        reqtask.state = RequestState::WAITING_RESPONSE;
        while(true){
            bool flag = false;
            if(reqtask.state == RequestState::WAITING_RESPONSE || reqtask.state == RequestState::WAITING_FILE){
                srv->send_lock.lock();
                flag = srv->send_mes(new_sess, reqtask);
                srv->send_lock.unlock();
            }
            if(!flag) break;
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

string Server::recv_mes(SOCKET s){
    int part = 0;
    memset(recv_buf, 0, Config::BUFFERLENGTH);
    stringstream recv_stream;
    int bytes_num = recv(s, recv_buf, Config::BUFFERLENGTH, 0);
    if(bytes_num == 0 | bytes_num == SOCKET_ERROR){
        closesocket(s);
        cout << "WARNING : invlida socket" << s << '\n';
    } else {
        cout << "\nINFO : HTTP request information from socket:" << s << "\n";
        recv_stream.write(recv_buf, bytes_num);
        cout << recv_stream.str() << endl;
    }
    return recv_stream.str();
}

void Server::remove_sess(){
    auto it_b = remove(sess_threads.begin(), sess_threads.end(), nullptr);
    sess_threads.erase(it_b, sess_threads.end());
}

bool Server::send_mes(SOCKET s, RequestTask& rt){
    if(rt.state != RequestState::FINISH){
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
        
        if( bytes_num == SOCKET_ERROR | bytes_num == 0){
            closesocket(s);
            cout << "ERROR :  send error" << endl;
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
            cout << "INFO : finish send:" << rt.get->get_req_file() << "\n\n";
            cout << "WARNING : close socket" << s << '\n';
            closesocket(s);
        }
        return true;
    } else return false;
}

string Server::cache_file(SOCKET s){
    return Config::CACHE + "\\recv_temp" + to_string(s) + ".txt";
}