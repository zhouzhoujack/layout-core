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

set(LAYOUTIO_STAGE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "layout-io 产物输出根目录")
set(LAYOUTIO_STAGE_LIB    "${LAYOUTIO_STAGE_ROOT}/lib")
set(LAYOUTIO_STAGE_INCLUDE "${LAYOUTIO_STAGE_ROOT}/include")

set_target_properties(data PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${LAYOUTIO_STAGE_LIB}"
)

file(GLOB_RECURSE DATA_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_custom_command(TARGET data POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LAYOUTIO_STAGE_INCLUDE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DATA_HEADERS} "${LAYOUTIO_STAGE_INCLUDE}"
    COMMENT "Staging data headers -> ${LAYOUTIO_STAGE_INCLUDE}"
)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
    add_subdirectory(test)
endif()
