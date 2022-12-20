function(CUTEST_ADD_TEST name)
    add_executable(${name} "${name}.c")
    setup_target_wall(${name})
    target_link_libraries(${name} PRIVATE cutest)
    add_test(${name} ${name})
endfunction()

CUTEST_ADD_TEST(empty)
CUTEST_ADD_TEST(simple)
