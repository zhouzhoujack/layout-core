cmake_minimum_required(VERSION 3.16)

project(layout-core)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("/home/hz/toolchain/cmake/toolchain.cmake")

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

find_package(ZLIB REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Xml)
find_package(toolkit REQUIRED)

enable_testing()

# ---------------------------------------------------------------------------
# 版本号与安装规则
#
# compile.sh --ver <x.y.z> 通过 -DLAYOUTCORE_VERSION 传入（默认 0.1.0）。
# 产物用 cmake --install 安装到 <prefix>，与 toolkit 一样提供 CMake 包：
#
#   <prefix>/include/*.h                        公共头（平铺）
#   <prefix>/lib/lib{utils,data,db-io,io}.so
#   <prefix>/lib/cmake/layout-core/             find_package(layout-core)
#
#   find_package(layout-core REQUIRED COMPONENTS utils data db-io io)
#   target_link_libraries(app PRIVATE layout-core::db-io layout-core::io)
#
# 包模板见 cmake/layout-coreConfig.cmake。
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if (NOT LAYOUTCORE_VERSION)
    set(LAYOUTCORE_VERSION "0.1.0")
endif()

add_subdirectory(util)
add_subdirectory(data)
add_subdirectory(geo)
add_subdirectory(io)

# ---------------------------------------------------------------------------
# 头文件安装：公共 API -> <prefix>/include，其余 -> <prefix>/include/detail
#
# 只有下面列出的头算公共 API；其它头都是实现细节，统一进 detail/。
# 公共头对 detail 头的引用会在“安装副本”里改写成 #include "detail/xxx.h"
# （源码目录不动），因此消费方只要 -I<prefix>/include 就能编译，
# include/detail 不需要出现在搜索路径里。
# ---------------------------------------------------------------------------
set(LAYOUTCORE_PUBLIC_HEADERS
    geoBox.h
    geoCell.h
    geoCommonReader.h
    geoLayerProperties.h
    geoLayout.h
    geoLoadLayoutOptions.h
    geoPolygon.h
    geoReader.h
    geoRegion.h
    geoSaveLayoutOptions.h
    geoShapes.h
    geoStreamLayers.h
    geoWriter.h
    utilsStream.h
)

file(GLOB_RECURSE LAYOUTCORE_ALL_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/util/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/data/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/geo/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/*.h")

set(LAYOUTCORE_PUBLIC_HEADER_FILES "")
set(LAYOUTCORE_DETAIL_HEADERS "")
foreach (_hdr IN LISTS LAYOUTCORE_ALL_HEADERS)
    get_filename_component(_hdr_name "${_hdr}" NAME)
    list(FIND LAYOUTCORE_PUBLIC_HEADERS "${_hdr_name}" _public_index)
    if (_public_index EQUAL -1)
        list(APPEND LAYOUTCORE_DETAIL_HEADERS "${_hdr}")
    else ()
        list(APPEND LAYOUTCORE_PUBLIC_HEADER_FILES "${_hdr}")
    endif()
endforeach()

# 公共头必须都能找到（避免改名后忘了同步清单）
foreach (_name IN LISTS LAYOUTCORE_PUBLIC_HEADERS)
    set(_found FALSE)
    foreach (_hdr IN LISTS LAYOUTCORE_PUBLIC_HEADER_FILES)
        get_filename_component(_hdr_name "${_hdr}" NAME)
        if (_hdr_name STREQUAL _name)
            set(_found TRUE)
        endif()
    endforeach()
    if (NOT _found)
        message(FATAL_ERROR "layout-core: 公共头 ${_name} 未找到，请同步 LAYOUTCORE_PUBLIC_HEADERS")
    endif()
endforeach()

# 生成改写后的公共头（放 <build>/include/layout-core，不动源码）
set(LAYOUTCORE_STAGED_INCLUDE "${PROJECT_BINARY_DIR}/include/layout-core")
file(MAKE_DIRECTORY "${LAYOUTCORE_STAGED_INCLUDE}")
foreach (_hdr IN LISTS LAYOUTCORE_PUBLIC_HEADER_FILES)
    get_filename_component(_hdr_name "${_hdr}" NAME)
    file(READ "${_hdr}" _hdr_text)
    foreach (_detail IN LISTS LAYOUTCORE_DETAIL_HEADERS)
        get_filename_component(_detail_name "${_detail}" NAME)
        string(REPLACE
            "#include \"${_detail_name}\""
            "#include \"detail/${_detail_name}\""
            _hdr_text "${_hdr_text}")
    endforeach()
    file(WRITE "${LAYOUTCORE_STAGED_INCLUDE}/${_hdr_name}" "${_hdr_text}")
endforeach()

install(FILES ${LAYOUTCORE_DETAIL_HEADERS}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/detail")

install(DIRECTORY "${LAYOUTCORE_STAGED_INCLUDE}/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h")

# ---------------------------------------------------------------------------
# 导出包
#
# 各模块里的 install(TARGETS <lib> EXPORT LAYOUTCORETargets ...) 汇总到这里，
# 生成 layout-coreTargets.cmake（命名空间 layout-core::）。
# ---------------------------------------------------------------------------
install(EXPORT LAYOUTCORETargets
    FILE      layout-coreTargets.cmake
    NAMESPACE layout-core::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/layout-core")

# 生成在 <build>/cmake/layout-core/ 下：若安装前缀恰好是构建目录，
# 写在 prefix 根上的 config 会遮住安装的版本。
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/layout-coreConfig.cmake"
    "${PROJECT_BINARY_DIR}/cmake/layout-core/layout-coreConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/layout-core")

write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/cmake/layout-core/layout-coreConfigVersion.cmake"
    VERSION "${LAYOUTCORE_VERSION}"
    COMPATIBILITY SameMajorVersion)

install(FILES
    "${PROJECT_BINARY_DIR}/cmake/layout-core/layout-coreConfig.cmake"
    "${PROJECT_BINARY_DIR}/cmake/layout-core/layout-coreConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/layout-core")

