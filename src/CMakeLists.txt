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

