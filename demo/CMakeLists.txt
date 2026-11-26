cmake_minimum_required(VERSION 3.16)

project(layout-core-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("/home/hz/toolchain/cmake/toolchain.cmake")

# demo 像第三方用户一样消费“已安装”的 layout-core 包（find_package +
# layout-core::xxx），而不是链接源码树里的 target，因此它同时是导出树
# （headers / libs / cmake 包）的冒烟测试。
#
#   ./compile.sh --demo            # 构建 + 安装到 ./layout-core_0_1_0 后运行 demo
#   # 等价于：
#   cmake -S demo -B build-demo -DCMAKE_PREFIX_PATH=<prefix>
#   cmake --build build-demo -j3
find_package(layout-core REQUIRED COMPONENTS utils data db-io io)

add_executable(layout-core-demo
    main.cpp
    demoCase.cpp
)
target_link_libraries(layout-core-demo PRIVATE layout-core::db-io layout-core::io)

# io 只做格式注册（静态初始化），demo 不直接引用其符号，
# 需要 --no-as-needed 防止链接器把它丢掉。
target_link_options(layout-core-demo PRIVATE "-Wl,--no-as-needed")
