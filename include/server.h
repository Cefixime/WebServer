#ifndef SERVER_INC
#define SERVER_INC
#include <WinSock2.h>
#include <string>
#include <fstream>
#define UNBLOCKING 1


class RequestInfo{
public:
    std::string file;
};

class Server{
private:
    friend class RequestInfo;
	SOCKET srv_socket;			//服务器socket
    SOCKET session_socket;      //会话socket
	char* recv_buf;				//接受缓冲区
    char* send_buf;             //发送缓冲区
    bool ready_send;            //是否可发送
    RequestInfo req;            //请求信息
	fd_set* rfds;				//用于检查socket是否有数据到来的的文件描述符，用于socket非阻塞模式下等待网络事件通知（有数据到来）
	fd_set* wfds;				//用于检查socket是否可以发送的文件描述符，用于socket非阻塞模式下等待网络事件通知（可以发送数据）
	sockaddr_in srv_addr;		//服务器端IP地址
    int socket_signal;          //socket信号，表示有事件发生
    u_long* block_mode;         //阻塞模式

    // 直接存到缓存区
    void recv_mes(SOCKET s);     

    // 累计存到文件中                 
    // void recv_mes(SOCKET s, std::string file_path);   

    // 发送文件 
    // void send_mes(SOCKET s, std::string file_path);  
    
    // 获取IP地址   
    std::string  get_addr(SOCKET s);

    // 获取端口
    int get_port(SOCKET s);                             
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