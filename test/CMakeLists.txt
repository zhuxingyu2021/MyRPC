include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/cereal/include)

macro(make_test subdir fname)
    add_executable(${fname} ${subdir}/${fname}.cpp)
    target_link_libraries(${fname} MyRPC)
endmacro()

make_test(module_fiber_test test_fiber)
make_test(module_fiber_test test_fiberpool)
make_test(module_fiber_test test_hooksleep)
make_test(module_fiber_test test_hookio)
make_test(module_fiber_test test_hooksocketio)

make_test(module_net_test test_serializer)
make_test(module_net_test test_deserializer)
make_test(module_net_test test_utils_byteorder)
make_test(module_net_test test_json_file_parser)
make_test(module_net_test test_tcp_server_client)

make_test(module_rpc_test test_config)
