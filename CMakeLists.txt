cmake_minimum_required(VERSION 3.16)
project(MyRPC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(3rdparty/spdlog)
add_subdirectory(3rdparty/cereal)

add_subdirectory(src)

add_subdirectory(test)