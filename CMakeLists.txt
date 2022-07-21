cmake_minimum_required(VERSION 3.16)
project(myrpc)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(spdlog)

add_subdirectory(src)

add_subdirectory(test)