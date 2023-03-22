include_directories(${PROJECT_SOURCE_DIR}/include)

set(SRCFILE
        debug.cpp
        utils.cpp

        fiber/hook_io.cpp
        fiber/hook_sleep.cpp
        fiber/fiber.cpp
        fiber/event_manager.cpp
        fiber/fiber_pool.cpp
        fiber/fiber_sync.cpp
        fiber/ctx_switch.S

        buffer/stringbuffer.cpp
        buffer/ringbuffer.cpp

        net/inetaddr.cpp
        net/tcp_server.cpp
        net/tcp_client.cpp

        rpc/jsonrpc/jsonrpc_server_base.cpp

)

find_package(Threads REQUIRED)

add_library(MyRPC STATIC ${SRCFILE})

if(${WITH_ZOOKEEPER})
    target_link_libraries(MyRPC zookeeper spdlog dl)
else()
    target_link_libraries(MyRPC spdlog dl)
endif()