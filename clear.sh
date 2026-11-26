#!/usr/bin/env bash
# ===========================================================================
# 清理 layout-core 的构建产物
#
# 删除：
#   build/        cmake -S src -B build 的构建目录
#   build-*/      demo 或其它自定义构建目录（如 build-demo/）
#   lib/          编译产物 *.so
#   include/      平铺暂存的头文件
#   layout-core_*/  ./compile.sh --ver <x.y.z> 生成的版本目录
#
# 用法：
#   ./clear.sh              清理上述全部内容
#   ./clear.sh -n           只列出将删除的目录，不做实际删除
# ===========================================================================
set -euo pipefail

cd "$(dirname "$0")"

dry_run=0
for arg in "$@"; do
    case "${arg}" in
        -n|--dry-run) dry_run=1 ;;
        -h|--help)
            echo "用法: ./clear.sh [-n|--dry-run]"
            exit 0
            ;;
        *)
            echo "未知参数: ${arg}" >&2
            echo "用法: ./clear.sh [-n|--dry-run]" >&2
            exit 1
            ;;
    esac
done

targets=()
for d in build build-* lib include layout-core_*; do
    # 没有匹配项时通配符会原样返回，靠 -d 判断过滤掉
    [[ -d "${d}" ]] && targets+=("${d}")
done

if [[ ${#targets[@]} -eq 0 ]]; then
    echo "没有需要清理的内容。"
    exit 0
fi

for t in "${targets[@]}"; do
    if [[ "${dry_run}" == "1" ]]; then
        echo "  [dry-run] rm -rf ${t}/"
    else
        rm -rf "${t}"
        echo "  已删除 ${t}/"
    fi
done

if [[ "${dry_run}" == "0" ]]; then
    echo
    echo "layout-core 已清理。重新构建： ./compile.sh [--test] [--demo]"
fi
