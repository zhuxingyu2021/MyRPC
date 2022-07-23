include_directories(${PROJECT_SOURCE_DIR}/include)

set(SRCFILE
        hookio.cpp
        hooksleep.cpp
        macro.cpp
        fiber.cpp
        eventmanager.cpp
        fiberpool.cpp
)

find_package(Boost COMPONENTS coroutine REQUIRED)
find_package(Threads REQUIRED)

add_library(MyRPC STATIC ${SRCFILE})

target_link_libraries(MyRPC spdlog ${Boost_LIBRARIES} dl)