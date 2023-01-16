
set(test_case_list
    all_assertion
    arg_count
    custom_type
    empty
    filter
    list_parameterized_as_int
    list_parameterized_as_string
    list_parameterized_as_struct
    simple)

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
