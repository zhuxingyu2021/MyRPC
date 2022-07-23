include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(testfiber moduletest/testfiber.cpp)
target_link_libraries(testfiber MyRPC)

add_executable(testfiberpool moduletest/testfiberpool.cpp)
target_link_libraries(testfiberpool MyRPC)

add_executable(testhooksleep moduletest/testhooksleep.cpp)
target_link_libraries(testhooksleep MyRPC)

add_executable(testhookio moduletest/testhookio.cpp)
target_link_libraries(testhookio MyRPC -static-libgcc -static-libstdc++)
