
add_library(test_runtime
    "string_matrix.c"
    "test.c"
)
target_include_directories(test_runtime
    PUBLIC
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)
cutest_setup_target_wall(test_runtime)

function(test_setup_test_case)
    set(prefix TESTCASE)
    set(options OPTIONAL FAST)
    set(singleValues TARGET)
    set(multiValues SOURCES LINK CFLAGS)

    include(CMakeParseArguments)
    cmake_parse_arguments(${prefix}
        "${options}"
        "${singleValues}"
        "${multiValues}"
        ${ARGN}
    )

    add_executable(${TESTCASE_TARGET}
        ${PROJECT_SOURCE_DIR}/src/cutest.c
        ${TESTCASE_SOURCES}
    )
    target_link_libraries(${TESTCASE_TARGET} PRIVATE
        test_runtime
        ${TESTCASE_LINK}
    )
    target_compile_options(${TESTCASE_TARGET} PRIVATE
        ${TESTCASE_CFLAGS}
    )
    target_include_directories(${TESTCASE_TARGET}
        PUBLIC
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    )
    cutest_setup_target_wall(${TESTCASE_TARGET})
    add_test(NAME ${TESTCASE_TARGET} COMMAND ${TESTCASE_TARGET})
endfunction()

set(test_case_list
    cmd_also_run_disabled_tests
    cmd_filter
    cmd_help
    cmd_list_tests_list_parameterized_as_int
    cmd_list_tests_list_parameterized_as_string
    cmd_list_tests_list_parameterized_as_struct
    cmd_list_types
    cmd_repeat
    cmd_shuffle
    feature_all_assertion
    feature_assertion_failure
    feature_barg
    feature_current_test
    feature_custom_type
    feature_empty
    feature_failure_print
    feature_hook_balance
    feature_manual_register
    feature_narg
    feature_print
    feature_simple
)

foreach(x IN LISTS test_case_list)
    test_setup_test_case(TARGET ${x}
        SOURCES case/${x}.c)
endforeach()

test_setup_test_case(TARGET porting_abort
    SOURCES case/porting_abort.c
    CFLAGS -DCUTEST_PORTING_ABORT
)

test_setup_test_case(TARGET porting_clock_gettime
    SOURCES case/porting_clock_gettime.c
    CFLAGS -DCUTEST_PORTING_CLOCK_GETTIME
)

test_setup_test_case(TARGET porting_cvfprintf
    SOURCES case/porting_cvfprintf.c
    CFLAGS -DCUTEST_PORTING_CVFPRINTF
)

test_setup_test_case(TARGET porting_gettid
    SOURCES case/porting_gettid.c
    CFLAGS -DCUTEST_PORTING_GETTID
)

test_setup_test_case(TARGET porting_setjmp
    SOURCES case/porting_setjmp.c
    CFLAGS -DCUTEST_PORTING_SETJMP
)
