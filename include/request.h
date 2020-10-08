#ifndef REQUEST_INC
#define REQUEST_INC
#include <string>
#include <fstream>
enum class RequestState{
    UNSOLVE,
    IN_STARTING,
    WAITING_RESPONSE,
    FINISH,
};

class RequestInfo{
public:
    RequestState state = RequestState::UNSOLVE;
    int file_length;
    int file_pos;                       //文件位置
    std::string file_path;
    std::string file_type;
    std::ifstream* file_stream;
};

#endif