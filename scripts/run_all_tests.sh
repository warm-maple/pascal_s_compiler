#!/usr/bin/env bash

# Pascal-S 编译器 - 全量回归测试脚本（带答案对比）
# 用法：./scripts/run_all_tests.sh [编译器路径] [测试集目录] [答案文件]

# 配置
COMPILER="${1:-./pascal-s-compiler}"
TEST_DIR="${2:-open_set}"
ANSWER_FILE="${3:-$TEST_DIR/test_data.json}"
OUTPUT_DIR="test_output"
C_OUTPUT_DIR="$OUTPUT_DIR/c_code"
EXECUTABLE_DIR="$OUTPUT_DIR/executables"
RESULT_DIR="$OUTPUT_DIR/results"
ERROR_DIR="$OUTPUT_DIR/errors"
ANSWER_DIR="$OUTPUT_DIR/answers"

# 颜色
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 计数器
total=0
passed=0
compile_failed=0
c_compile_failed=0
runtime_failed=0
output_mismatch=0

# 创建目录
mkdir -p "$C_OUTPUT_DIR" "$EXECUTABLE_DIR" "$RESULT_DIR" "$ERROR_DIR" "$ANSWER_DIR"

# 清空
rm -f "$RESULT_DIR"/*.txt "$ERROR_DIR"/*.txt "$ANSWER_DIR"/*.expected "$ANSWER_DIR"/*.input

# 加载答案
if [ -f "$ANSWER_FILE" ]; then
    echo "加载答案文件：$ANSWER_FILE"
    if command -v python3 &> /dev/null; then
        python3 -c "
import json, os
with open('$ANSWER_FILE') as f: data = json.load(f)
ad = '$ANSWER_DIR'
for i in data:
    fn = i['filename'].replace('.c', '')
    open(ad + '/' + fn + '.expected', 'w').write(i['expected'])
    open(ad + '/' + fn + '.input', 'w').write(i.get('input', ''))
print('已加载', len(data), '个答案')
"
    elif command -v node &> /dev/null; then
        node -e "const fs=require('fs'),d=JSON.parse(fs.readFileSync('$ANSWER_FILE')),ad='$ANSWER_DIR';d.forEach(i=>{const f=i.filename.replace('.c','');fs.writeFileSync(ad+'/'+f+'.expected',i.expected);fs.writeFileSync(ad+'/'+f+'.input',i.input||'')});console.log('已加载',d.length,'个答案')"
    fi
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Pascal-S 编译器 - 全量回归测试${NC}"
echo -e "${BLUE}========================================${NC}"
echo "编译器：$COMPILER"
echo "测试目录：$TEST_DIR"
echo ""

[ ! -f "$COMPILER" ] && { echo -e "${RED}错误：编译器不存在${NC}"; exit 1; }

run_test() {
    local pf="$1" bn cfile exefile rfile efile inpfile expfile ansinp
    bn=$(basename "$pf" .pas)
    cfile="$C_OUTPUT_DIR/${bn}.c"
    exefile="$EXECUTABLE_DIR/${bn}"
    rfile="$RESULT_DIR/${bn}.txt"
    efile="$ERROR_DIR/${bn}.txt"
    inpfile="${pf%.pas}.in"
    expfile="$ANSWER_DIR/${bn}.expected"
    ansinp="$ANSWER_DIR/${bn}.input"
    
    total=$((total + 1))
    
    if ! "$COMPILER" -o "$cfile" "$pf" >/dev/null 2>"$efile"; then
        echo -e "${RED}[FAIL]${NC} $bn - Pascal 编译失败"
        compile_failed=$((compile_failed + 1))
        echo "PASCAL_COMPILE_ERROR" > "$rfile"
        return 1
    fi
    
    if ! gcc -std=c99 -w "$cfile" -o "$exefile" 2>>"$efile"; then
        echo -e "${YELLOW}[FAIL]${NC} $bn - C 编译失败"
        c_compile_failed=$((c_compile_failed + 1))
        echo "C_COMPILE_ERROR" > "$rfile"
        return 1
    fi
    
    local out=""
    if [ -f "$ansinp" ] && [ -s "$ansinp" ]; then
        out=$(cat "$ansinp" | "$exefile" 2>>"$efile") || {
            echo -e "${YELLOW}[FAIL]${NC} $bn - 运行时错误"
            runtime_failed=$((runtime_failed + 1))
            echo "RUNTIME_ERROR" > "$rfile"
            return 1
        }
    elif [ -f "$inpfile" ]; then
        out=$("$exefile" < "$inpfile" 2>>"$efile") || {
            echo -e "${YELLOW}[FAIL]${NC} $bn - 运行时错误"
            runtime_failed=$((runtime_failed + 1))
            echo "RUNTIME_ERROR" > "$rfile"
            return 1
        }
    else
        out=$("$exefile" 2>>"$efile") || {
            echo -e "${YELLOW}[FAIL]${NC} $bn - 运行时错误"
            runtime_failed=$((runtime_failed + 1))
            echo "RUNTIME_ERROR" > "$rfile"
            return 1
        }
    fi
    
    if [ -f "$expfile" ]; then
        local exp=$(cat "$expfile")
        local ot=$(echo "$out" | tr -d '\r' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        local et=$(echo "$exp" | tr -d '\r' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        if [ "$ot" = "$et" ]; then
            echo -e "${GREEN}[PASS]${NC} $bn"
            passed=$((passed + 1))
            echo "PASS" > "$rfile"
        else
            echo -e "${RED}[FAIL]${NC} $bn - 输出不匹配"
            echo "  预期：${et:0:60}..."
            echo "  实际：${ot:0:60}..."
            output_mismatch=$((output_mismatch + 1))
            echo "OUTPUT_MISMATCH" > "$rfile"
        fi
    else
        echo "$out" > "$rfile"
        echo -e "${GREEN}[PASS]${NC} $bn (无答案)"
        passed=$((passed + 1))
    fi
    return 0
}

echo "开始测试..."
echo ""

set +e
for pf in "$TEST_DIR"/*.pas; do
    [ -f "$pf" ] && run_test "$pf"
done
set -e

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  测试结果${NC}"
echo -e "${BLUE}========================================${NC}"
echo "总数：$total | 通过：$passed | 失败：$((total - passed))"
echo "  Pascal 编译失败：$compile_failed"
echo "  C 编译失败：$c_compile_failed"
echo "  运行时错误：$runtime_failed"
echo "  输出不匹配：$output_mismatch"

if [ $((total - passed)) -gt 0 ]; then
    echo ""
    echo -e "${YELLOW}失败列表:${NC}"
    for rf in "$RESULT_DIR"/*.txt; do
        [ -f "$rf" ] || continue
        bn=$(basename "$rf" .txt)
        res=$(cat "$rf")
        [ "$res" != "PASS" ] && [ "$res" != "PASS_NO_ANSWER" ] && [ -n "$res" ] && echo "  - $bn: $res"
    done
fi

pr=0
command -v bc &>/dev/null && pr=$(echo "scale=2; $passed * 100 / $total" | bc) || pr=$((passed * 100 / total))
ts=$(date +%Y-%m-%dT%H:%M:%S 2>/dev/null || date +%Y-%m-%d)

cat > "$RESULT_DIR/report.json" << EOF
{"timestamp":"$ts","compiler":"$COMPILER","total":$total,"passed":$passed,"failed":$((total-passed)),"compile_failed":$compile_failed,"c_compile_failed":$c_compile_failed,"runtime_failed":$runtime_failed,"output_mismatch":$output_mismatch,"pass_rate":"${pr}%"}
EOF

echo ""
echo "报告：$RESULT_DIR/report.json"
[ $passed -eq $total ] && { echo -e "${GREEN}全部通过！${NC}"; exit 0; } || { echo -e "${RED}部分失败${NC}"; exit 1; }
