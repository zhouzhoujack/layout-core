#!/usr/bin/env bash
# ===========================================================================
# 编译 layout-core（独立的 GDS2 / OASIS 读入 + polygon 布尔库）
#
# 源码布局：src/（工程根，含 src/CMakeLists.txt）、
#           src/util/（→ libutils.so）、src/util/test/（ut_utils）、
#           src/geo/（→ libdb-io.so）、src/geo/test/（ut_geo）、
#           demo/（独立工程，纯可执行 demo，演示如何用 lib/ 与 include/）
#
# 用法：
#   ./compile.sh                                    # 默认 Debug 构建到 build/
#   ./compile.sh --release                          # Release 构建
#   ./compile.sh --test                             # 构建后跑 ctest 单元测试
#   ./compile.sh --demo                             # 构建并运行 demo/ 示例程序
#
# 注意：layout-core 依赖 Qt5（Core/Gui/Xml），产物需要 Qt5 运行库。
# 工具链与第三方库路径在 src/CMakeLists.txt 里通过
# include("/home/hz/toolchain/cmake/toolchain.cmake") 引入，无需额外参数。
# ===========================================================================
set -euo pipefail

cd "$(dirname "$0")"

build_type="Debug"
run_tests=0
run_demo=0
build_args=()
for arg in "$@"; do
    case "${arg}" in
        --release) build_type="Release" ;;
        --test) run_tests=1 ;;
        --demo) run_demo=1 ;;
        *) build_args+=("${arg}") ;;   # 透传给 cmake --build
    esac
done

rm -rf build/
cmake -S src -B build -DCMAKE_BUILD_TYPE="${build_type}" ${CMAKE_ARGS:-}

if [[ ${#build_args[@]} -gt 0 ]]; then
    cmake --build build -j3 "${build_args[@]}"
else
    cmake --build build -j3
fi

echo
echo "layout-core built (${build_type}):"
ls -1 lib/*.so 2>/dev/null | sed 's/^/  /'
echo "  include: $(ls include/*.h 2>/dev/null | wc -l) headers"

if [[ "${run_tests}" == "1" ]]; then
    echo
    ctest --test-dir build --output-on-failure
fi

# 示例程序：demo/ 是独立工程，只用 lib/ 与 include/（不使用 gtest）
if [[ "${run_demo}" == "1" ]]; then
    echo
    echo "构建并运行 demo ..."
    rm -rf build-demo
    cmake -S demo -B build-demo > /dev/null
    cmake --build build-demo -j3
    echo
    ./build-demo/layout-core-demo
fi
