
set(test_lists
    empty
    filter
    simple)

foreach(x IN LISTS test_lists)
    add_executable(${x} ${x}.c)
    setup_target_wall(${x})
    target_link_libraries(${x} PRIVATE cutest)
    add_test(NAME ${x} COMMAND ${x})
endforeach()
