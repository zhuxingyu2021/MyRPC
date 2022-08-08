include_directories(${PROJECT_SOURCE_DIR}/include)

set(SRCFILE
        fiber/hookio.cpp
        fiber/hooksleep.cpp
        fiber/fiber.cpp
        fiber/eventmanager.cpp
        fiber/fiberpool.cpp

        net/inetaddr.cpp
        net/tcpserver.cpp
        net/tcpclient.cpp

        macro.cpp
        utils.cpp
        stringbuffer.cpp
)

find_package(Boost COMPONENTS coroutine REQUIRED)
find_package(Threads REQUIRED)

add_library(MyRPC STATIC ${SRCFILE})

target_link_libraries(MyRPC spdlog ${Boost_LIBRARIES} dl)