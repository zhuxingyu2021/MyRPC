include_directories(${PROJECT_SOURCE_DIR}/include)

set(SRCFILE
        macro.cpp
        utils.cpp
        stringbuffer.cpp

        fiber/hook_io.cpp
        fiber/hook_sleep.cpp
        fiber/fiber.cpp
        fiber/event_manager.cpp
        fiber/fiber_pool.cpp

        net/inetaddr.cpp
        net/tcp_server.cpp

        rpc/config.cpp
        rpc/rpc_registry_server.cpp
)

find_package(Boost COMPONENTS coroutine REQUIRED)
find_package(Threads REQUIRED)

add_library(MyRPC STATIC ${SRCFILE})

target_link_libraries(MyRPC spdlog ${Boost_LIBRARIES} dl)