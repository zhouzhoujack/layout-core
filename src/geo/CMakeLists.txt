cmake_minimum_required(VERSION 3.16)

file(GLOB DB_IO_SOURCES
    "base/*.cc"
    "collections/*.cc"
    "ops/*.cc"
    "hier/*.cc"
)

list(FILTER DB_IO_SOURCES EXCLUDE REGEX ".*/hier/(geoTilingProcessor|geoLayoutQuery)\\.cc$")

add_library(db-io SHARED
    ${DB_IO_SOURCES}
)

target_include_directories(db-io PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/base>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/collections>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ops>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/hier>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(db-io PUBLIC
    utils
    data
    ZLIB::ZLIB
)

target_link_options(db-io PRIVATE -Wl,--no-undefined)

# 安装：<prefix>/lib/libdb-io.so + <prefix>/include/*.h（平铺）+ 导出目标
install(TARGETS db-io
    EXPORT  LAYOUTCORETargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(test)
