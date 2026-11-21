cmake_minimum_required(VERSION 3.16)

file(GLOB READER_SOURCES
    "*.cc"
)

# 目录暂为空时不创建目标，避免 "No SOURCES given to target"；
# 放入 .cc 后自动生效（重新 configure 即可）。
if(NOT READER_SOURCES)
    message(STATUS "reader: no sources yet, target skipped")
    return()
endif()

add_library(reader SHARED
    ${READER_SOURCES}
)

target_include_directories(reader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(reader PUBLIC
    db-io
    writer
)

target_compile_definitions(reader PUBLIC HAVE_QT)

target_link_options(reader PRIVATE -Wl,--no-undefined)

set(LAYOUTIO_STAGE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "layout-io 产物输出根目录")
set(LAYOUTIO_STAGE_LIB    "${LAYOUTIO_STAGE_ROOT}/lib")
set(LAYOUTIO_STAGE_INCLUDE "${LAYOUTIO_STAGE_ROOT}/include")

set_target_properties(reader PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${LAYOUTIO_STAGE_LIB}"
)

file(GLOB_RECURSE READER_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_custom_command(TARGET reader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LAYOUTIO_STAGE_INCLUDE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${READER_HEADERS} "${LAYOUTIO_STAGE_INCLUDE}"
    COMMENT "Staging reader headers -> ${LAYOUTIO_STAGE_INCLUDE}"
)
