#ifndef REQUEST_INC
#define REQUEST_INC
#include <string>
#include <fstream>
#include <winsock2.h>
#include "myhttp.h"
enum class RequestState{
    UNSOLVE,
    IN_STARTING,
    WAITING_RESPONSE,
    WAITING_FILE,
    FINISH,
};

class RequestTask{
private:
    friend class Server;
    RequestState state{RequestState::UNSOLVE};
    int file_length{0};
    int offset{0};
    std::ifstream* file_stream{nullptr};
    HttpResponseHeader* res{nullptr};
    HttpGetHeader* get{nullptr};
    // 解析报头
    void parse(std::string header);

    // 准备文件
    void prepare_file();

public:
    RequestTask() = default;
    ~RequestTask();
};

#endif