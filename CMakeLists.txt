cmake_minimum_required(VERSION 3.2)
project(cutest)

###############################################################################
# Options
###############################################################################
option(CUTEST_NO_C99_SUPPORT
    "Disable C99 support."
    OFF)
option(CUTEST_NO_LONGLONG_SUPPORT
    "Disable long long support."
    OFF)
option(CUTEST_NO_ULONGLONG_SUPPORT
    "Disable unsigned long long support."
    OFF)
option(CUTEST_NO_INT8_SUPPORT
    "Disable int8_t support."
    OFF)
option(CUTEST_NO_UINT8_SUPPORT
    "Disable uint8_t support."
    OFF)
option(CUTEST_NO_INT16_SUPPORT
    "Disable uint16_t support."
    OFF)
option(CUTEST_NO_UINT16_SUPPORT
    "Disable uint16_t support."
    OFF)
option(CUTEST_NO_INT32_SUPPORT
    "Disable int32_t support."
    OFF)
option(CUTEST_NO_UINT32_SUPPORT
    "Disable uint32_t support."
    OFF)
option(CUTEST_NO_INT64_SUPPORT
    "Disable int64_t support."
    OFF)
option(CUTEST_NO_UINT64_SUPPORT
    "Disable uint64_t support."
    OFF)
option(CUTEST_NO_SIZE_SUPPORT
    "Disable size_t support."
    OFF)
option(CUTEST_NO_PTRDIFF_SUPPORT
    "Disable ptrdiff_t support."
    OFF)
option(CUTEST_NO_INTPTR_SUPPORT
    "Disable inttpr_t support."
    OFF)
option(CUTEST_NO_UINTPTR_SUPPORT
    "Disable uinttpr_t support."
    OFF)

###############################################################################
# Functions
###############################################################################

# Enable all reasonable warnings and make all warnings into errors.
function(setup_target_wall name)
    if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${name} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    endif ()
endfunction()

###############################################################################
# Setup library
###############################################################################

add_library(${PROJECT_NAME}
    "src/cutest.c")

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

setup_target_wall(${PROJECT_NAME})

if (CUTEST_NO_C99_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_C99_SUPPORT)
endif ()
if (CUTEST_NO_LONGLONG_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_LONGLONG_SUPPORT)
endif ()
if (CUTEST_NO_ULONGLONG_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_ULONGLONG_SUPPORT)
endif ()
if (CUTEST_NO_INT8_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_INT8_SUPPORT)
endif ()
if (CUTEST_NO_UINT8_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_UINT8_SUPPORT)
endif ()
if (CUTEST_NO_INT16_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_INT16_SUPPORT)
endif ()
if (CUTEST_NO_UINT16_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_UINT16_SUPPORT)
endif ()
if (CUTEST_NO_INT32_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_INT32_SUPPORT)
endif ()
if (CUTEST_NO_UINT32_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_UINT32_SUPPORT)
endif ()
if (CUTEST_NO_INT64_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_INT64_SUPPORT)
endif ()
if (CUTEST_NO_UINT64_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_UINT64_SUPPORT)
endif ()
if (CUTEST_NO_SIZE_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_SIZE_SUPPORT)
endif ()
if (CUTEST_NO_PTRDIFF_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_PTRDIFF_SUPPORT)
endif ()
if (CUTEST_NO_INTPTR_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_INTPTR_SUPPORT)
endif ()
if (CUTEST_NO_UINTPTR_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_UINTPTR_SUPPORT)
endif ()

###############################################################################
# Dependency
###############################################################################

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Threads::Threads)

###############################################################################
# Test
###############################################################################

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(example)
    add_subdirectory(test)
endif()
