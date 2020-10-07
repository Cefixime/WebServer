#ifndef CONFIG_INC
#define CONFIG_INC
#include <WinSock2.h>
#include <string>

class Config{
public:
	static int MAXCONNECTION;		    //最大连接数
	static int BUFFERLENGTH;		    //缓冲区大小
    static std::string SERVERADDRESS;   //服务器地址
	static int PORT;				    //服务器端口
    static std::string DIRECTORY;       // 目录
private:
    Config() = default;
	~Config() = default;
};
#endif