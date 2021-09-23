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

add_executable(automata
        src/main.cpp
        src/automaton.cpp
        src/regex.cpp)

add_executable(automata_test
        test/test.cpp
        src/automaton.cpp
        src/regex.cpp
        )

