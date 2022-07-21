include_directories(${PROJECT_SOURCE_DIR}/include)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SRCDIR)

find_package(Boost COMPONENTS coroutine REQUIRED)
find_package(Threads REQUIRED)

add_library(myrpc STATIC ${SRCDIR})

target_link_libraries(myrpc  spdlog ${Boost_LIBRARIES})