cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)

file(GLOB TL_CORE_SOURCES
    "core/*.cc"
    "object/*.cc"
    "exception/*.cc"
    "io/*.cc"
    "text/*.cc"
)

add_library(utils SHARED
    ${TL_CORE_SOURCES}
)

target_include_directories(utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/object>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/exception>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/io>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/text>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(utils PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Xml
    toolkit::logkit
    ZLIB::ZLIB
)

target_link_options(utils PRIVATE -Wl,--no-undefined)

# 安装：<prefix>/lib/libutils.so + <prefix>/include/*.h（平铺）+ 导出目标
install(TARGETS utils
    EXPORT  LAYOUTCORETargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

file(GLOB_RECURSE TL_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${TL_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

add_subdirectory(test)
