include_directories(${PROJECT_SOURCE_DIR}/include)

macro(make_test subdir fname)
    add_executable(${fname} ${subdir}/${fname}.cpp)
    target_link_libraries(${fname} MyRPC)
endmacro()

make_test(module_fiber_test testfiber)
make_test(module_fiber_test testfiberpool)
make_test(module_fiber_test testhooksleep)
make_test(module_fiber_test testhookio)
make_test(module_fiber_test testhooksocketio)
