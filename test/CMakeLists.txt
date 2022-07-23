include_directories(${PROJECT_SOURCE_DIR}/include)

macro(make_moduletest fname)
    add_executable(${fname} moduletest/${fname}.cpp)
    target_link_libraries(${fname} MyRPC)
endmacro()

make_moduletest(testfiber)
make_moduletest(testfiberpool)
make_moduletest(testhooksleep)
make_moduletest(testhookio)
make_moduletest(testhooksocketio)
