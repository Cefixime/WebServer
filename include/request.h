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

class RequestTask{
public:
    RequestState state = RequestState::UNSOLVE;
    int file_length;
    int file_pos;                       //文件位置
    std::ifstream* file_stream;
};

#endif