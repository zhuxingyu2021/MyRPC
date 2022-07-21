include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(testfiber moduletest/testfiber.cpp)
target_link_libraries(testfiber myrpc)
