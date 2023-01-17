cmake_minimum_required(VERSION 3.2)
project(cutest)

###############################################################################
# Options
###############################################################################
option(CUTEST_NO_COLORFUL_SUPPORT
    "Do not detect colorful support and always use pain text."
    OFF)
option(CUTEST_NO_MULTITHREADING_SUPPORT
    "Disable multithreading support."
    OFF)
option(CUTEST_NO_TIME_BASED_SHUFFLE
    "If --test_shuffle is used, use 0 instead of time(NULL) as default seed."
    OFF)
option(CUTEST_NO_ELAPSED_TIME_MEASUREMENT
    "Disable elapsed time measurement."
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

if (CUTEST_NO_COLORFUL_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_COLORFUL_SUPPORT)
endif()
if (CUTEST_NO_MULTITHREADING_SUPPORT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_MULTITHREADING_SUPPORT)
endif ()
if (CUTEST_NO_TIME_BASED_SHUFFLE)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_TIME_BASED_SHUFFLE)
endif ()
if (CUTEST_NO_ELAPSED_TIME_MEASUREMENT)
    target_compile_options(${name} PRIVATE -DCUTEST_NO_ELAPSED_TIME_MEASUREMENT)
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
