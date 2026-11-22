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
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/object
    ${CMAKE_CURRENT_SOURCE_DIR}/exception
    ${CMAKE_CURRENT_SOURCE_DIR}/io
    ${CMAKE_CURRENT_SOURCE_DIR}/text
)

target_link_libraries(utils PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Xml
    glog::glog
    toolkit::toolkit
    ZLIB::ZLIB
)

target_link_options(utils PRIVATE -Wl,--no-undefined)

# LAYOUTCORE_STAGE_ROOT / LIB / INCLUDE 由顶层 src/CMakeLists.txt 定义

set_target_properties(utils PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${LAYOUTCORE_STAGE_LIB}"
)

file(GLOB_RECURSE TL_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_custom_command(TARGET utils POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LAYOUTCORE_STAGE_INCLUDE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TL_HEADERS} "${LAYOUTCORE_STAGE_INCLUDE}"
    COMMENT "Staging utils headers -> ${LAYOUTCORE_STAGE_INCLUDE}"
)

add_subdirectory(test)
