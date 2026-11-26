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
#   ./compile.sh                                    # 默认 Debug，产物在 ./layout-core_0_1_0
#   ./compile.sh --release                          # Release 构建
#   ./compile.sh --test                             # 构建后跑 ctest 单元测试
#   ./compile.sh --demo                             # 构建并运行 demo/ 示例程序
#
# 产物目录（<dir>/lib/*.so + <dir>/include/*.h + <dir>/lib/cmake/layout-core）：
#   ./compile.sh --path /tmp/out                    # 精确输出到 /tmp/out
#   ./compile.sh --ver 1.2.3                        # 输出到 ./layout-core_1_2_3
#   ./compile.sh --ver 1.2.3 /opt/third_party       # 输出到 /opt/third_party/layout-core_1_2_3
#   优先级：--path > third_party_dir > 仓库根；--ver 默认 0.1.0，仅影响目录名
#
# 注意：layout-core 依赖 Qt5（Core/Gui/Xml），产物需要 Qt5 运行库。
# 工具链与第三方库路径在 src/CMakeLists.txt 里通过
# include("/home/hz/toolchain/cmake/toolchain.cmake") 引入，无需额外参数。
# ===========================================================================
set -euo pipefail

cd "$(dirname "$0")"

usage() {
    cat <<'EOF'
用法: ./compile.sh [选项] [third_party_dir]

  --release | --debug    构建类型（默认 Debug）
  --test                 构建后运行 ctest
  --demo                 构建并运行 demo/
  --ver <x.y.z>          版本号（默认 0.1.0），决定 layout-core_<ver> 目录名
  --path <dir>           精确指定产物目录（优先级最高）
  -h, --help             显示本帮助

产物目录（lib/ 与 include/ 直接放在其下）：
  无参数                 <repo>/layout-core_0_1_0
  --ver 1.2.3            <repo>/layout-core_1_2_3
  --ver 1.2.3 <dir>      <dir>/layout-core_1_2_3
  --path <dir>           <dir>

兜底：'--' 之后的参数原样传给 cmake --build。
EOF
}

build_type="Debug"
run_tests=0
run_demo=0
version="0.1.0"
out_path=""
third_party_dir=""
build_args=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --release) build_type="Release"; shift ;;
        --debug)   build_type="Debug";   shift ;;
        --test)    run_tests=1; shift ;;
        --demo)    run_demo=1; shift ;;
        --ver)
            if [[ $# -lt 2 ]]; then
                echo "compile.sh: --ver 需要一个版本号，如 --ver 1.2.3" >&2
                exit 1
            fi
            version="$2"; shift 2 ;;
        --ver=*)   version="${1#--ver=}"; shift ;;
        --path)
            if [[ $# -lt 2 ]]; then
                echo "compile.sh: --path 需要一个目录，如 --path /tmp/out" >&2
                exit 1
            fi
            out_path="$2"; shift 2 ;;
        --path=*)  out_path="${1#--path=}"; shift ;;
        -h|--help) usage; exit 0 ;;
        --) shift; build_args+=("$@"); break ;;
        -*)
            echo "compile.sh: 未知选项 '$1'（-h 查看用法）" >&2
            exit 1 ;;
        *)
            if [[ -n "${third_party_dir}" ]]; then
                echo "compile.sh: 多余的参数 '$1'（只接受一个 third_party_dir）" >&2
                exit 1
            fi
            third_party_dir="$1"; shift ;;
    esac
done

if [[ -z "${version}" ]]; then
    echo "compile.sh: --ver 不能为空" >&2
    exit 1
fi

pkg_dir="layout-core_${version//./_}"

# 产物根目录优先级：--path > third_party_dir > 仓库根（<repo>/layout-core_<ver>）
if [[ -n "${out_path}" ]]; then
    stage_root="${out_path}"
elif [[ -n "${third_party_dir}" ]]; then
    stage_root="${third_party_dir}/${pkg_dir}"
else
    stage_root="${pkg_dir}"
fi

# cmake 的 mkdir/copy 按构建目录解析相对路径，这里统一换成绝对路径
if [[ "${stage_root}" != /* ]]; then
    stage_root="$(pwd)/${stage_root}"
fi
if [[ "${stage_root}" != "/" ]]; then
    stage_root="${stage_root%/}"      # 去掉用户传入的尾部 /
    stage_root="${stage_root%/.}"     # 仓库根这个默认值会得到 ".../."
fi

rm -rf build/
cmake -S src -B build -DCMAKE_BUILD_TYPE="${build_type}" \
    -DLAYOUTCORE_VERSION="${version}" ${CMAKE_ARGS:-}

if [[ ${#build_args[@]} -gt 0 ]]; then
    cmake --build build -j3 "${build_args[@]}"
else
    cmake --build build -j3
fi

# cmake --install 的相对 --prefix 会按构建目录解析，前面已统一成绝对路径
cmake --install build --prefix "${stage_root}" > /dev/null

echo
echo "layout-core ${version} installed (${build_type}):"
echo "  root:    ${stage_root}"
ls -1 "${stage_root}/lib/"*.so 2>/dev/null | sed 's/^/  /'
echo "  include: $(ls "${stage_root}/include/"*.h 2>/dev/null | wc -l) headers"
echo "  cmake:   ${stage_root}/lib/cmake/layout-core"

if [[ "${run_tests}" == "1" ]]; then
    echo
    ctest --test-dir build --output-on-failure
fi

# 示例程序：demo/ 是独立工程，只用 lib/ 与 include/（不使用 gtest）
if [[ "${run_demo}" == "1" ]]; then
    echo
    echo "构建并运行 demo ..."
    rm -rf build-demo
    cmake -S demo -B build-demo -DCMAKE_PREFIX_PATH="${stage_root}" > /dev/null
    cmake --build build-demo -j3
    echo
    ./build-demo/layout-core-demo
fi
