add_executable(cutest_test
    tools/__init__.c
    tools/help.c
    main.c
    test.c
)
target_include_directories(cutest_test
    PUBLIC
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(cutest_test
    PRIVATE
        cutest
)
cutest_setup_target_wall(cutest_test)

add_test(NAME cutest_test COMMAND $<TARGET_FILE:cutest_test>)
