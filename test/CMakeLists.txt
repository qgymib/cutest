
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
    feature_arg_count
    feature_assertion_failure
    feature_current_test
    feature_custom_type
    feature_empty
    feature_hook_balance
    feature_simple)

add_library(test_runtime
    src/string_matrix.c
    src/test.c)
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
setup_target_wall(test_runtime)
target_link_libraries(test_runtime PUBLIC cutest)

foreach(x IN LISTS test_case_list)
    add_executable(${x} src/case/${x}.c)
    setup_target_wall(${x})
    target_link_libraries(${x} PRIVATE test_runtime)
    add_test(NAME ${x} COMMAND ${x})
endforeach()
