cmake_minimum_required(VERSION 3.16)

project(layoutio-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("/home/hz/toolchain/cmake/toolchain.cmake")

get_filename_component(LAYOUTIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(LAYOUTIO_LIB_DIR     "${LAYOUTIO_ROOT}/lib")
set(LAYOUTIO_INCLUDE_DIR "${LAYOUTIO_ROOT}/include")

foreach (_f lib/libdb-io.so lib/libio.so lib/libutils.so lib/libdata.so include/geoRegion.h)
    if (NOT EXISTS "${LAYOUTIO_ROOT}/${_f}")
        message(FATAL_ERROR "未找到 ${LAYOUTIO_ROOT}/${_f}，请先在 layout-io 目录执行 ./compile.sh")
    endif()
endforeach()

find_package(ZLIB REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Xml)
find_package(glog REQUIRED)
find_package(toolkit REQUIRED)

function (layoutio_import name)
    add_library(layoutio::${name} SHARED IMPORTED)
    set_target_properties(layoutio::${name} PROPERTIES
        IMPORTED_LOCATION "${LAYOUTIO_LIB_DIR}/lib${name}.so"
        INTERFACE_INCLUDE_DIRECTORIES "${LAYOUTIO_INCLUDE_DIR}"
        INTERFACE_COMPILE_DEFINITIONS HAVE_QT
        INTERFACE_LINK_LIBRARIES "${ARGN}")
endfunction()

layoutio_import(utils Qt5::Core Qt5::Gui Qt5::Xml glog::glog toolkit::toolkit ZLIB::ZLIB)
layoutio_import(data layoutio::utils)
layoutio_import(db-io layoutio::data layoutio::utils)
layoutio_import(io layoutio::data)

add_executable(layoutio_demo
    main.cpp
    demoCase.cpp
)
target_link_libraries(layoutio_demo PRIVATE layoutio::db-io layoutio::io)

# io 只做格式注册（静态初始化），demo 不直接引用其符号，
# 需要 --no-as-needed 防止链接器把它丢掉。
target_link_options(layoutio_demo PRIVATE "-Wl,--no-as-needed")
