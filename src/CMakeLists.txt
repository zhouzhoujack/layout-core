cmake_minimum_required(VERSION 3.16)

project(layout-io)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("/home/hz/toolchain/cmake/toolchain.cmake")

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

find_package(ZLIB REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Xml)
find_package(glog REQUIRED)
find_package(toolkit REQUIRED)

enable_testing()

add_subdirectory(util)
add_subdirectory(data)
add_subdirectory(geo)
add_subdirectory(io)

