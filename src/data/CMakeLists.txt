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
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/basic
    ${CMAKE_CURRENT_SOURCE_DIR}/layout
)

target_link_libraries(data PUBLIC
    utils
)

target_link_options(data PRIVATE -Wl,--no-undefined)

# LAYOUTCORE_STAGE_ROOT / LIB / INCLUDE 由顶层 src/CMakeLists.txt 定义

set_target_properties(data PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${LAYOUTCORE_STAGE_LIB}"
)

file(GLOB_RECURSE DATA_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_custom_command(TARGET data POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LAYOUTCORE_STAGE_INCLUDE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DATA_HEADERS} "${LAYOUTCORE_STAGE_INCLUDE}"
    COMMENT "Staging data headers -> ${LAYOUTCORE_STAGE_INCLUDE}"
)


add_subdirectory(test)

