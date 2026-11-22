cmake_minimum_required(VERSION 3.16)

# ===========================================================================
# IO 层（单一库 libio.so）
#
#   common/ —— 读写共用的格式定义（geoGDS2*.h / geoOASIS2*.h）
#   reader/ —— GDS2 / OASIS 读取实现与格式注册
#   writer/ —— GDS2 / OASIS 写入实现
#
# 全部编进同一个 SHARED 库 io，只依赖 data（及 utils）。
# 库内静态注册：格式声明（reader）、writer 工厂（writer），
# 使用方需用 --no-as-needed 链接，避免静态初始化被丢弃。
# ===========================================================================

file(GLOB IO_COMMON_SOURCES CONFIGURE_DEPENDS 
"${CMAKE_CURRENT_SOURCE_DIR}/common/*.cc"
)

file(GLOB IO_READER_SOURCES CONFIGURE_DEPENDS 
"${CMAKE_CURRENT_SOURCE_DIR}/reader/*.cc"
)

file(GLOB IO_WRITER_SOURCES CONFIGURE_DEPENDS 
"${CMAKE_CURRENT_SOURCE_DIR}/writer/*.cc"
)

file(GLOB_RECURSE IO_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/reader/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/writer/*.h"
)

# LAYOUTIO_STAGE_ROOT / LIB / INCLUDE 由顶层 src/CMakeLists.txt 定义

set(IO_SOURCES
    ${IO_COMMON_SOURCES}
    ${IO_READER_SOURCES}
    ${IO_WRITER_SOURCES}
)

if(NOT IO_SOURCES)
    message(STATUS "io: no sources yet, target skipped")
    return()
endif()

add_library(io SHARED
    ${IO_SOURCES}
)

target_include_directories(io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/reader
    ${CMAKE_CURRENT_SOURCE_DIR}/writer
)

target_link_libraries(io PUBLIC
    data
)

target_compile_definitions(io PUBLIC HAVE_QT)

target_link_options(io PRIVATE -Wl,--no-undefined)

set_target_properties(io PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${LAYOUTIO_STAGE_LIB}"
)

add_custom_command(TARGET io POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LAYOUTIO_STAGE_INCLUDE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${IO_HEADERS} "${LAYOUTIO_STAGE_INCLUDE}"
    COMMENT "Staging io headers -> ${LAYOUTIO_STAGE_INCLUDE}"
)

add_subdirectory(test)
