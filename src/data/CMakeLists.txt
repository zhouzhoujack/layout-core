cmake_minimum_required(VERSION 3.16)

file(GLOB DATA_SOURCES
    "*.cc"
    "common/*.cc"
    "basic/*.cc"
    "layout/*.cc"
)

add_library(data SHARED
    ${DATA_SOURCES}
)

target_include_directories(data PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/common>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/basic>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/layout>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(data PUBLIC
    utils
)

target_link_options(data PRIVATE -Wl,--no-undefined)

# 安装：<prefix>/lib/libdata.so + <prefix>/include/*.h（平铺）+ 导出目标
install(TARGETS data
    EXPORT  LAYOUTCORETargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

file(GLOB_RECURSE DATA_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${DATA_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

add_subdirectory(test)

