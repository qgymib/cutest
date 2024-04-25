add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cutest_h_hex.c
    COMMAND $<TARGET_FILE:hex_dump>
        --input=${PROJECT_SOURCE_DIR}/include/cutest.h
        --output=${CMAKE_CURRENT_BINARY_DIR}/cutest_h_hex.c
        --name=cutest_h_hex
    DEPENDS
        $<TARGET_FILE:hex_dump>
        ${PROJECT_SOURCE_DIR}/include/cutest.h
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cutest_c_hex.c
    COMMAND $<TARGET_FILE:hex_dump>
        --input=${PROJECT_SOURCE_DIR}/src/cutest.c
        --output=${CMAKE_CURRENT_BINARY_DIR}/cutest_c_hex.c
        --name=cutest_c_hex
    DEPENDS
        $<TARGET_FILE:hex_dump>
        ${PROJECT_SOURCE_DIR}/src/cutest.c
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/readme_md_hex.c
    COMMAND $<TARGET_FILE:hex_dump>
        --input=${PROJECT_SOURCE_DIR}/README.md
        --output=${CMAKE_CURRENT_BINARY_DIR}/readme_md_hex.c
        --name=readme_md_hex
    DEPENDS
        $<TARGET_FILE:hex_dump>
        ${PROJECT_SOURCE_DIR}/README.md
)

add_executable(doc_verify
    "test.c"
    "case/mainpage.c"
    "utils/foreachline.c"
    "utils/strtok_f.c"
    ${CMAKE_CURRENT_BINARY_DIR}/cutest_h_hex.c
    ${CMAKE_CURRENT_BINARY_DIR}/cutest_c_hex.c
    ${CMAKE_CURRENT_BINARY_DIR}/readme_md_hex.c
)
target_include_directories(doc_verify
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)
target_link_libraries(doc_verify
    PRIVATE
        cutest
)
cutest_setup_target_wall(doc_verify)

add_test(NAME doc_verify COMMAND $<TARGET_FILE:doc_verify>)
