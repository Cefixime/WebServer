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
    // rfds = new fd_set();
    // wfds = new fd_set();
    // block_mode = new u_long(UNBLOCKING);
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
    sockaddr_in client_addr;
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10;
    int client_addrlen = sizeof(client_addr);
    while (true) {
        remove_invalid_sockets();
        auto new_sess = accept(srv_socket, (LPSOCKADDR)&client_addr, &client_addrlen);

        if(new_sess != INVALID_SOCKET){
            sess_sockets.push_back(new_sess);
            cout << "INFO : session connect : " << '\"' << get_addr(new_sess) + ":" << get_port(new_sess) <<" socket:" <<new_sess << "\"\n";
        }
        for(auto sess:sess_sockets){
            if(req_map.find(sess) == req_map.end())
                req_map[sess] = RequestTask();
            socket_signal--;
            req_map[sess].parse(recv_mes(sess));
            req_map[sess].prepare_file();
            req_map[sess].state = RequestState::WAITING_RESPONSE;
            while(true){
                bool flag = false;
                if(req_map[sess].state == RequestState::WAITING_RESPONSE || req_map[sess].state == RequestState::WAITING_FILE){
                    flag = send_mes(sess, req_map[sess]);
                }
                if(!flag) break;
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

string Server::recv_mes(SOCKET s){
    int part = 0;
    memset(recv_buf, 0, Config::BUFFERLENGTH);
    stringstream recv_stream;
    int bytes_num = recv(s, recv_buf, Config::BUFFERLENGTH, 0);
    if(bytes_num == 0 | bytes_num == SOCKET_ERROR){
        closesocket(s);
        cout << "WARNING : invlida socket" << s << '\n';
        invalid_sockets.push_back(s);
    } else {
        cout << "INFO : HTTP request information from socket:" << s << "\n";
        recv_stream.write(recv_buf, bytes_num);
        cout << recv_stream.str() << endl;
    }
    return recv_stream.str();
}

void Server::recv_mes(SOCKET s, string file_path){
    memset(recv_buf, 0, Config::BUFFERLENGTH);
    stringstream recv_stream;
    ofstream outfile;
    int bytes_num = recv(s, recv_buf, Config::BUFFERLENGTH, 0);
    if(bytes_num == 0 | bytes_num == SOCKET_ERROR){
        closesocket(s);
        cout << "WARNING : invlida socket" << s << '\n';
        invalid_sockets.push_back(s);
    } else {
        outfile.open(file_path, ios::app|ios::binary);
        // cout << "INFO :  receive " << bytes_num << " bytes from" <<" socket:"<< s << endl;
        outfile.write(recv_buf, bytes_num);
        outfile.close();
        if(bytes_num < Config::BUFFERLENGTH){
            ifstream infile(file_path);
            cout << "INFO : HTTP request information\n";
            cout << infile.rdbuf();
        }
    }
}

void Server::remove_invalid_sockets(){
    for(auto& s: invalid_sockets){
        sess_sockets.remove(s);
        auto key = req_map.find(s);
        if(key != req_map.end())
            req_map.erase(req_map.find(s));
    }
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
            invalid_sockets.push_back(s);
            cout << "ERROR :  send error" << endl;
        } else {
            // cout << "INFO :  send " << bytes_num << " bytes to socket:"<<s <<"\n";
            if(rt.state == RequestState::WAITING_RESPONSE){
                rt.state = RequestState::WAITING_FILE;
                // cout << "INFO : send response header to socket:" << s <<"\n";
            } else {
                rt.offset += read_size;
            }
        }
        if(rt.offset == rt.file_length){
            rt.state = RequestState::FINISH;
            rt.file_stream->close();
            cout << "INFO : finish send:" << rt.get->get_req_file() << '\n';
            cout << "WARNING : close socket" << s << '\n';
            closesocket(s);
            invalid_sockets.push_back(s);
        }
        return true;
    } else return false;
}

string Server::cache_file(SOCKET s){
    return Config::CACHE + "\\recv_temp" + to_string(s) + ".txt";
}