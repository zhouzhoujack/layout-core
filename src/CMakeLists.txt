cmake_minimum_required(VERSION 3.16)

project(layout-core)

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
find_package(toolkit REQUIRED)

enable_testing()

# ---------------------------------------------------------------------------
# 产物输出根目录（仓库根）
#
# 统一在这里用普通变量定义（绝对、规范化路径），各模块只引用
# LAYOUTCORE_STAGE_LIB / LAYOUTCORE_STAGE_INCLUDE。
# 注意：不要改成 CACHE 变量——缓存里一旦存了旧目录（例如改名前的
# src/utils/../..），mkdir/copy 会按 "utils/.." 把旧目录重新创建出来。
# ---------------------------------------------------------------------------
get_filename_component(LAYOUTCORE_STAGE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(LAYOUTCORE_STAGE_LIB     "${LAYOUTCORE_STAGE_ROOT}/lib")
set(LAYOUTCORE_STAGE_INCLUDE "${LAYOUTCORE_STAGE_ROOT}/include")

add_subdirectory(util)
add_subdirectory(data)
add_subdirectory(geo)
add_subdirectory(io)

