#include <iostream>
#include <string>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <thread>
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
    stop_signal = new bool(false);
    block_mode = new u_long(UNBLOCKING);
}

Server::~Server(){
    closesocket(srv_socket);
    if(recv_buf != nullptr)
        delete[] recv_buf;
    if(send_buf != nullptr)
        delete[] send_buf;
    if(srv_socket != INVALID_SOCKET)
        srv_socket = INVALID_SOCKET;
    for(auto& socket: sess_sockets){
        closesocket(socket);
    }
    delete rfds;
    delete wfds;
    delete stop_signal;
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
    //非阻塞模式
    if(ioctlsocket(srv_socket, FIONBIO, block_mode) == SOCKET_ERROR) {     
        cout << "ERROR : ioctlsocket() for new session failed with error!\n";
        return -1;
    }
    thread t(Server::Stopservice, stop_signal);
    while (true) {
        // 设置socket集合
        if(*stop_signal) break;
        FD_ZERO(rfds);
        FD_ZERO(wfds);

        FD_SET(srv_socket, rfds);

        remove_invalid_sockets();

        for(auto &sess: sess_sockets){
            FD_SET(sess, rfds);
            FD_SET(sess, wfds);

        }

        // 接受信号
        socket_signal = select(0, rfds, wfds, NULL, &tv);
        if(socket_signal == SOCKET_ERROR){
            cout << "ERROR :  select error!" << endl;
            cout << WSAGetLastError();
            return -1;
        }




        if(FD_ISSET(srv_socket, rfds)){
            socket_signal--;
            
            // 接受连接
            auto new_sess = accept(srv_socket, (LPSOCKADDR)&client_addr, &client_addrlen);

            if(new_sess != INVALID_SOCKET){
                sess_sockets.push_back(new_sess);
                cout << "INFO :  session connect : " << '\"' << get_addr(new_sess) + ":" << get_port(new_sess) <<" socket:" <<new_sess << "\"\n";
                // 设置非阻塞
                if(ioctlsocket(new_sess, FIONBIO, block_mode) == SOCKET_ERROR){
                    cout << "ERROR :  ioctlsocket() for new session failed with error!\n";
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
                    socket_signal--;
                    recv_mes(sess, cache_file(sess));
                    auto &req = req_map[sess];
                    if(req.state == RequestState::UNSOLVE || req.state == RequestState::FINISH)
                        req.state = RequestState::IN_STARTING;
                } else if(sess != srv_socket && req_map[sess].state == RequestState::IN_STARTING){
                    req_map[sess].state = RequestState::WAITING_RESPONSE;
                    req_map[sess].parse(sess);
                    remove(cache_file(sess).c_str());                   // 删除报头文件
                    req_map[sess].prepare_file();
                }

                if(FD_ISSET(sess, wfds)){
                    socket_signal--;
                    if(req_map[sess].state == RequestState::WAITING_RESPONSE || req_map[sess].state == RequestState::WAITING_FILE){
                        send_mes(sess, req_map[sess]);
                    }
                }
            }
        }
    }
    t.join();
    return 1;
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
    ofstream outfile;
    int bytes_num = recv(s, recv_buf, Config::BUFFERLENGTH, 0);
    if(bytes_num == 0 | bytes_num == SOCKET_ERROR){
        closesocket(s);
        cout << "WARNING :  invlida socket" << s << '\n';
        invalid_sockets.push_back(s);
    } else {
        outfile.open(file_path, ios::app|ios::binary);
        cout << "INFO :  receive " << bytes_num << " bytes from" <<" socket:"<< s << endl;
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
    invalid_sockets.clear();
}

void Server::send_mes(SOCKET s, RequestTask& rt){
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
            if(rt.state == RequestState::WAITING_RESPONSE){
                rt.state = RequestState::WAITING_FILE;
            } else {
                rt.offset += read_size;
            }
        }
        if(rt.offset == rt.file_length){
            rt.state = RequestState::FINISH;
            rt.file_stream->close();
            cout << "WARNING : close socket" << s << '\n';
            closesocket(s);
            invalid_sockets.push_back(s);
        }
    }
}

string Server::cache_file(SOCKET s){
    return Config::CACHE + "\\recv_temp" + to_string(s) + ".txt";
}

void Server::Stopservice(bool* signal){
    string temp;
    while(true){
        cin >> temp;
        if(temp == "quit")
            break;
    }
    *signal = true;
    cout << "WARNING : STOP SERVICE" << endl;
}
