cmake_minimum_required(VERSION 3.16)
project(MyRPC LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(3rdparty/spdlog)

if(${WITH_ZOOKEEPER})
    add_subdirectory(3rdparty/zookeeper/zookeeper-client/zookeeper-client-c)
    add_compile_definitions(ZOOKEEPER THREADED)
    include_directories(3rdparty/zookeeper/zookeeper-client/zookeeper-client-c/include)
endif()

include_directories(3rdparty/spdlog/include)

add_compile_definitions(INIT_STACK_SIZE=8192)
add_subdirectory(src)

add_subdirectory(test)
add_subdirectory(example)