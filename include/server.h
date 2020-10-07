#ifndef SERVER_INC
#define SERVER_INC
#include <WinSock2.h>
#include <string>
#include <fstream>
class Server{
private:
	SOCKET srv_socket;			//服务器socket
	char* recvBuf;				//接受缓冲区
	fd_set rfds;				//用于检查socket是否有数据到来的的文件描述符，用于socket非阻塞模式下等待网络事件通知（有数据到来）
	fd_set wfds;				//用于检查socket是否可以发送的文件描述符，用于socket非阻塞模式下等待网络事件通知（可以发送数据）
	sockaddr_in srvAddr;		//服务器端IP地址
    int socket_signal;          // socket信号，表示有事件发生
    void recv_mes(SOCKET s);                            // 直接存到缓存区
    void recv_mes(SOCKET s, std::string file_path);     // 累计存到文件中
    void send_mes(SOCKET s, std::string msg);           // 发送短消息
    void send_mes(SOCKET s, std::ifstream file_path);   // 发送文件
    std::string  get_addr(SOCKET s);                    // 获取IP地址
    int get_port(SOCKET s);                             // 获取端口
public:
    Server();
    ~Server();
    int WinsockStartup();		//初始化Winsock
    int ServerStartup();		//初始化Server，包括创建SOCKET，绑定到IP和PORT
    int ListenStartup();		//开始监听客户端请求
    int Loop();					//循环执行"等待客户端请求"->“向其他客户转发信息”->"等待客户端消息"
};
#endif