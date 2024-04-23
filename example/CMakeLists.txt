
add_executable(cutest_example
    "main.c"
    "test.c"
    "test_f.c"
    "test_p.c")

target_link_libraries(cutest_example PRIVATE cutest)
cutest_setup_target_wall(cutest_example)
