
add_executable(cutest_example
    main.c
    test_p.c)

target_link_libraries(cutest_example PRIVATE cutest)
setup_target_wall(cutest_example)
