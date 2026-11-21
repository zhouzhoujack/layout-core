cmake_minimum_required(VERSION 3.16)

file(GLOB DB_IO_SOURCES
    "base/*.cc"
    "core/*.cc"
    "collections/*.cc"
    "ops/*.cc"
    "hier/*.cc"
    "manager/*.cc"
    "netlist/*.cc"
    "io/*.cc"
    "gds/*.cc"
    "oas/*.cc"
)

list(FILTER DB_IO_SOURCES EXCLUDE REGEX ".*/hier/(geoTilingProcessor|geoLayoutQuery)\\.cc$")

add_library(db-io SHARED
    ${DB_IO_SOURCES}
)

target_include_directories(db-io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/base
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/collections
    ${CMAKE_CURRENT_SOURCE_DIR}/ops
    ${CMAKE_CURRENT_SOURCE_DIR}/hier
    ${CMAKE_CURRENT_SOURCE_DIR}/netlist
    ${CMAKE_CURRENT_SOURCE_DIR}/manager
    ${CMAKE_CURRENT_SOURCE_DIR}/io
    ${CMAKE_CURRENT_SOURCE_DIR}/gds
    ${CMAKE_CURRENT_SOURCE_DIR}/oas
    ${CMAKE_CURRENT_SOURCE_DIR}/gsi_shim
)

target_link_libraries(db-io PUBLIC
    utils
    data
    ZLIB::ZLIB
)

target_compile_definitions(db-io PUBLIC HAVE_QT)

target_link_options(db-io PRIVATE -Wl,--no-undefined)

set(LAYOUTIO_STAGE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "layout-io 产物输出根目录")
set(LAYOUTIO_STAGE_LIB    "${LAYOUTIO_STAGE_ROOT}/lib")
set(LAYOUTIO_STAGE_INCLUDE "${LAYOUTIO_STAGE_ROOT}/include")

set_target_properties(db-io PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${LAYOUTIO_STAGE_LIB}"
)

file(GLOB_RECURSE GEO_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_custom_command(TARGET db-io POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LAYOUTIO_STAGE_INCLUDE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEO_HEADERS} "${LAYOUTIO_STAGE_INCLUDE}"
    COMMENT "Staging geo headers -> ${LAYOUTIO_STAGE_INCLUDE}"
)

add_subdirectory(test)
