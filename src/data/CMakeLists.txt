cmake_minimum_required(VERSION 3.16)

file(GLOB DATA_SOURCES
    "*.cc"
)

add_library(data SHARED
    ${DATA_SOURCES}
)

target_include_directories(data PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(data PUBLIC
    utils
)

target_compile_definitions(data PUBLIC HAVE_QT)

target_link_options(data PRIVATE -Wl,--no-undefined)

# LAYOUTIO_STAGE_ROOT / LIB / INCLUDE 由顶层 src/CMakeLists.txt 定义

set_target_properties(data PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${LAYOUTIO_STAGE_LIB}"
)

file(GLOB_RECURSE DATA_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_custom_command(TARGET data POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LAYOUTIO_STAGE_INCLUDE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DATA_HEADERS} "${LAYOUTIO_STAGE_INCLUDE}"
    COMMENT "Staging data headers -> ${LAYOUTIO_STAGE_INCLUDE}"
)


add_subdirectory(test)

