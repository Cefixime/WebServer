#ifndef SERVER_INC
#define SERVER_INC
#include <WinSock2.h>
#include "request.h"
#include <list>
#include <vector>
#include <mutex>
#include <thread>
#include <string>
#include <fstream>
#define UNBLOCKING 1


class Server{
private:
    friend class RequestTask;
	SOCKET srv_socket;			                                        //服务器socket
	sockaddr_in srv_addr;		                                        //服务器端IP地址
    std::vector<std::thread*> sess_threads;   //会话线程
    // std::list<SOCKET> invalid_sockets;                               //失效的会话列表
    // std::map<SOCKET, RequestTask> req_map;                          //请求信息
    std::mutex recv_lock;
    std::mutex send_lock;
	char* recv_buf;				                                        //接受缓冲区
    char* send_buf;                                                     //发送缓冲区
    int erron;                                                          //错误号

    // 清理完成的会话
    void remove_sess();

    // 会话函数
    static void session_handler(Server* srv, SOCKET s);
    // 直接存到缓存区
    std::string recv_mes(SOCKET s);      

    // 发送文件 
    bool send_mes(SOCKET s, RequestTask& rt);  

    // 获取IP地址   
    std::string  get_addr(SOCKET s);

    // 获取端口
    int get_port(SOCKET s);    

    // 缓存的http报头文件
    static std::string cache_file(SOCKET s);
public:
    Server();
    ~Server();

    //初始化Winsock
    int WinsockStartup();	

    //初始化Server，包括创建SOCKET，绑定到IP和PORT	
    int ServerStartup();	

    //开始监听客户端请求	
    int ListenStartup();

    //循环执行"等待客户端请求"->“向其他客户转发信息”->"等待客户端消息"		
    int Loop();		

    // 结束Winsock			
    int WinsockStop();          
};

#endif