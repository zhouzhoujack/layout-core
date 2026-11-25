cmake_minimum_required(VERSION 3.16)

project(layout-core-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("/home/hz/toolchain/cmake/toolchain.cmake")

get_filename_component(LAYOUTCORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(LAYOUTCORE_LIB_DIR     "${LAYOUTCORE_ROOT}/lib")
set(LAYOUTCORE_INCLUDE_DIR "${LAYOUTCORE_ROOT}/include")

foreach (_f lib/libdb-io.so lib/libio.so lib/libutils.so lib/libdata.so include/geoRegion.h)
    if (NOT EXISTS "${LAYOUTCORE_ROOT}/${_f}")
        message(FATAL_ERROR "未找到 ${LAYOUTCORE_ROOT}/${_f}，请先在 layout-core 目录执行 ./compile.sh")
    endif()
endforeach()

find_package(ZLIB REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Xml)
find_package(glog REQUIRED)
find_package(toolkit REQUIRED)

function (layoutcore_import name)
    add_library(layoutcore::${name} SHARED IMPORTED)
    set_target_properties(layoutcore::${name} PROPERTIES
        IMPORTED_LOCATION "${LAYOUTCORE_LIB_DIR}/lib${name}.so"
        INTERFACE_INCLUDE_DIRECTORIES "${LAYOUTCORE_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${ARGN}")
endfunction()

layoutcore_import(utils Qt5::Core Qt5::Gui Qt5::Xml glog::glog toolkit::logkit ZLIB::ZLIB)
layoutcore_import(data layoutcore::utils)
layoutcore_import(db-io layoutcore::data layoutcore::utils)
layoutcore_import(io layoutcore::data)

add_executable(layout-core-demo
    main.cpp
    demoCase.cpp
)
target_link_libraries(layout-core-demo PRIVATE layoutcore::db-io layoutcore::io)

# io 只做格式注册（静态初始化），demo 不直接引用其符号，
# 需要 --no-as-needed 防止链接器把它丢掉。
target_link_options(layout-core-demo PRIVATE "-Wl,--no-as-needed")
