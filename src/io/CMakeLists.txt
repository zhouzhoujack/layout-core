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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/common>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/reader>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/writer>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(io PUBLIC
    data
)

target_link_options(io PRIVATE -Wl,--no-undefined)

# 安装：<prefix>/lib/libio.so + <prefix>/include/*.h（平铺）+ 导出目标
install(TARGETS io
    EXPORT  LAYOUTCORETargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

list(REMOVE_DUPLICATES IO_HEADERS)
install(FILES ${IO_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

add_subdirectory(test)
