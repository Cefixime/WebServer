cmake_minimum_required(VERSION 3.0.0)
project(WebServer)
link_libraries(wsock32 ws2_32)

set(INCLUDE_PATH ${PROJECT_SOURCE_DIR}/include)
message(STATUS "HEADER dir: " ${INCLUDE_PATH})
message(STATUS "SOURCE dir: " ${PROJECT_SOURCE_DIR})

include_directories(${INCLUDE_PATH})
aux_source_directory(./src SRC_LIST)

set(CXX_STANDARD 11)
message(STATUS "C++ standard:" ${CXX_STANDARD})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
message(STATUS "output executable file path:" ${EXECUTABLE_OUTPUT_PATH})

add_executable(${PROJECT_NAME} main.cpp ${SRC_LIST})