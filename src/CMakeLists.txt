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

        rpc/config.cpp
        rpc/jsonrpc/jsonrpc_server_base.cpp
        # rpc/rpc_session.cpp
        # rpc/rpc_registry_server.cpp
        # rpc/rpc_server.cpp
        # rpc/rpc_client.cpp
        # rpc/rpc_client_connection.cpp
)

find_package(Threads REQUIRED)

add_library(MyRPC STATIC ${SRCFILE})

target_link_libraries(MyRPC spdlog dl)