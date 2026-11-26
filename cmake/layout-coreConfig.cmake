# layout-core 包的 Config 文件。
#
# @PACKAGE_INIT@ 由 configure_package_config_file() 展开（见 src/CMakeLists.txt），
# 因此安装后的 <prefix>/lib/cmake/layout-core/layout-coreConfig.cmake 是可重定位的。
#
# 四个库各对应一个组件，导出目标带 layout-core:: 命名空间：
#   find_package(layout-core REQUIRED)
#   find_package(layout-core REQUIRED COMPONENTS utils data db-io io)
#   target_link_libraries(app PRIVATE layout-core::db-io layout-core::io)
@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

# 导出目标的接口里带着这些依赖：
#   Qt5            —— utils 的公共头用到 QString
#   ZLIB           —— db-io 链接 zlib
#   toolkit::logkit —— utils 的 LOGI/LOGW/LOGE/LOGF
find_dependency(Qt5 COMPONENTS Core Gui Xml)
find_dependency(ZLIB)
find_dependency(toolkit COMPONENTS logkit)

# 组件记账：包里的每个库对应一个组件（等价于 toolkit 的 kit 列表）。
set(layout-core_utils_FOUND TRUE)
set(layout-core_data_FOUND TRUE)
set(layout-core_db-io_FOUND TRUE)
set(layout-core_io_FOUND TRUE)

include("${CMAKE_CURRENT_LIST_DIR}/layout-coreTargets.cmake")

check_required_components(layout-core)
