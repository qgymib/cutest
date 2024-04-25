add_executable(cutest_test
    cases/__init__.c
    cases/cmd_also_run_disabled_tests.c
    tools/__init__.c
    tools/eolcheck.c
    tools/help.c
    utils/file.c
    utils/regex.c
    utils/str.c
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

add_test(NAME cutest_eol_h
    COMMAND $<TARGET_FILE:cutest_test> -- eolcheck
        --file=${CMAKE_CURRENT_SOURCE_DIR}/../include/cutest.h
        --eol=LF
)
add_test(NAME cutest_eol_c
    COMMAND $<TARGET_FILE:cutest_test> -- eolcheck
        --file=${CMAKE_CURRENT_SOURCE_DIR}/../src/cutest.c
        --eol=LF
)
