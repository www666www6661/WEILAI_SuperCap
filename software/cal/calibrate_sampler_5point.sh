#!/usr/bin/env zsh

set -euo pipefail

POINT_COUNT=5

usage() {
    cat <<'USAGE'
用法：
  0) zsh 直接执行：
     zsh ./calibrate_sampler_5point.sh

用法：
  1) 交互输入：
     ./calibrate_sampler_5point.sh

  2) 命令行直接传入 5 组点：
     ./calibrate_sampler_5point.sh \
       <adc1> <real1> \
       <adc2> <real2> \
       <adc3> <real3> \
       <adc4> <real4> \
       <adc5> <real5>

说明：
  拟合模型为：real = k * adc + b
  脚本使用“5点画直线法”的最小二乘一次线性拟合，输出 dev_sampler.h 中可用的：
      float k;
      float b;

示例：
  ./calibrate_sampler_5point.sh 100 1.02 800 7.95 1500 14.91 2200 21.80 3000 29.74
USAGE
}

is_number() {
    [[ "$1" =~ '^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)$' ]]
}

render_points_table() {
    local current_index="${1:-0}"
    local current_field="${2:-}"
    local idx adc_val real_val marker

    if [[ -t 1 ]]; then
        printf '\033[2J\033[H'
    fi

    echo "请输入 ${POINT_COUNT} 组校准点（adc_val, real_value）"
    echo ""
    printf '%-6s | %-16s | %-16s | %-6s\n' "序号" "adc_val" "real_value" "状态"
    printf '%-6s-+-%-16s-+-%-16s-+-%-6s\n' "------" "----------------" "----------------" "------"

    for ((idx = 1; idx <= POINT_COUNT; idx++)); do
        adc_val="${adc_vals[idx]:-}"
        real_val="${real_vals[idx]:-}"
        marker=""

        if [[ -n "$adc_val" && -n "$real_val" ]]; then
            marker="已填"
        elif [[ $idx -eq $current_index ]]; then
            if [[ "$current_field" == "adc" ]]; then
                marker="录入adc"
            else
                marker="录入real"
            fi
        else
            marker="待填"
        fi

        printf '%-6s | %-16s | %-16s | %-6s\n' \
            "$idx" \
            "${adc_val:--}" \
            "${real_val:--}" \
            "$marker"
    done
    echo ""
}

declare -a adc_vals=()
declare -a real_vals=()

if [[ ${1:-} == "-h" || ${1:-} == "--help" ]]; then
    usage
    exit 0
fi

if [[ $# -eq 0 ]]; then
    for ((i = 1; i <= POINT_COUNT; i++)); do
        while true; do
            render_points_table "$i" "adc"
            read -r "adc?第${i}组 adc_val: "
            render_points_table "$i" "real"
            read -r "real?第${i}组 real_value: "

            if ! is_number "$adc"; then
                echo "adc_val 必须是数字，请重新输入。"
                continue
            fi
            if ! is_number "$real"; then
                echo "real_value 必须是数字，请重新输入。"
                continue
            fi

            adc_vals+=("$adc")
            real_vals+=("$real")
            render_points_table "$((i + 1))" "adc"
            break
        done
    done
elif [[ $# -eq $((POINT_COUNT * 2)) ]]; then
    for ((i = 1; i <= $#; i += 2)); do
        adc="${argv[i]}"
        next_index=$((i + 1))
        real="${argv[next_index]}"

        if ! is_number "$adc"; then
            echo "参数错误：$adc 不是合法的 adc_val 数字。" >&2
            exit 1
        fi
        if ! is_number "$real"; then
            echo "参数错误：$real 不是合法的 real_value 数字。" >&2
            exit 1
        fi

        adc_vals+=("$adc")
        real_vals+=("$real")
    done
else
    echo "参数数量错误。需要 0 个参数（交互输入）或 10 个参数（5组点）。" >&2
    usage
    exit 1
fi

adc_csv=$(IFS=,; echo "${adc_vals[*]}")
real_csv=$(IFS=,; echo "${real_vals[*]}")

awk -v adc_csv="$adc_csv" -v real_csv="$real_csv" '
BEGIN {
    n = split(adc_csv, x, ",")
    m = split(real_csv, y, ",")

    if (n != m || n != 5) {
        print "输入点数量必须为 5 组。" > "/dev/stderr"
        exit 1
    }

    sum_x = 0
    sum_y = 0
    sum_xx = 0
    sum_xy = 0

    for (i = 1; i <= n; i++) {
        sum_x += x[i]
        sum_y += y[i]
        sum_xx += x[i] * x[i]
        sum_xy += x[i] * y[i]
    }

    denom = n * sum_xx - sum_x * sum_x
    if (denom == 0) {
        print "拟合失败：5个 adc_val 不能全部相同。" > "/dev/stderr"
        exit 1
    }

    k = (n * sum_xy - sum_x * sum_y) / denom
    b = (sum_y - k * sum_x) / n

    print "校准结果："
    printf("k = %.10f\n", k)
    printf("b = %.10f\n", b)
    print ""
    print "可直接填入 Src/dev/dev_sampler.h 对应参数："
    printf("    .k = %.10f,\n", k)
    printf("    .b = %.10f,\n", b)
}
'
