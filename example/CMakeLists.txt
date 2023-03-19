include_directories(${PROJECT_SOURCE_DIR}/include)

macro(make_example fname)
    add_executable(${fname} ${fname}.cpp)
    target_link_libraries(${fname} MyRPC)
endmacro()

make_example(rpc_server)
