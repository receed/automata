cmake_minimum_required(VERSION 3.17)
project(automata)

include(ExternalProject)
find_package(Git REQUIRED)

ExternalProject_Add(
        doctest
        PREFIX ${CMAKE_BINARY_DIR}/doctest
        GIT_REPOSITORY https://github.com/onqtam/doctest.git
        TIMEOUT 10
        UPDATE_COMMAND ${GIT_EXECUTABLE} pull
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(doctest source_dir)
set(DOCTEST_INCLUDE_DIR ${source_dir}/doctest CACHE INTERNAL "Path to include folder for doctest")
include_directories(${DOCTEST_INCLUDE_DIR})

set(CMAKE_CXX_STANDARD 20)
include_directories(include)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" ${CMAKE_SOURCE_DIR}/cmake)
include(CodeCoverage)
SET(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
SET(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
SETUP_TARGET_FOR_COVERAGE(
        test_coverage  # Name for custom target.
        automata_test         # Name of the test driver executable that runs the tests.
        coverage            # Name of output directory.
)

add_executable(automata
        src/main.cpp
        src/automaton.cpp
        src/max_matching_prefix.cpp
        src/regex.cpp
        src/cli.cpp)

target_compile_options(automata PRIVATE "-DDOCTEST_CONFIG_DISABLE")

add_executable(automata_test
        test/test.cpp
        test/automaton_test.cpp
        test/regex_test.cpp
        src/automaton.cpp
        src/max_matching_prefix.cpp
        src/regex.cpp
        )

#add_compile_options(-Wall -Wextra -pedantic -Werror)
