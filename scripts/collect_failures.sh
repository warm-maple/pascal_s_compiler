#!/bin/bash

# Pascal-S 编译器 - 收集失败测试用例详情
# 用法：./scripts/collect_failures.sh

TEST_DIR="open_set"
OUTPUT_DIR="test_output"
C_OUTPUT_DIR="$OUTPUT_DIR/c_code"
RESULT_DIR="$OUTPUT_DIR/results"
ERROR_DIR="$OUTPUT_DIR/errors"
FAILURE_REPORT="$OUTPUT_DIR/failure_report.md"

echo "# Pascal-S 编译器 - 失败测试用例分析报告" > "$FAILURE_REPORT"
echo "" >> "$FAILURE_REPORT"
echo "生成时间：$(date)" >> "$FAILURE_REPORT"
echo "" >> "$FAILURE_REPORT"

# 收集失败的测试
failed_count=0

for result_file in "$RESULT_DIR"/*.txt; do
    if [ -f "$result_file" ]; then
        base_name=$(basename "$result_file" .txt)
        result=$(cat "$result_file")
        
        # 检查是否失败
        if [ "$result" = "PASCAL_COMPILE_ERROR" ] || [ "$result" = "C_COMPILE_ERROR" ] || [ "$result" = "RUNTIME_ERROR" ]; then
            failed_count=$((failed_count + 1))
            
            pas_file="$TEST_DIR/${base_name}.pas"
            c_file="$C_OUTPUT_DIR/${base_name}.c"
            error_file="$ERROR_DIR/${base_name}.txt"
            
            echo "## 测试用例：$base_name" >> "$FAILURE_REPORT"
            echo "" >> "$FAILURE_REPORT"
            
            # 错误类型
            case "$result" in
                "PASCAL_COMPILE_ERROR")
                    echo "**错误类型**: Pascal 编译失败" >> "$FAILURE_REPORT"
                    ;;
                "C_COMPILE_ERROR")
                    echo "**错误类型**: C 代码编译失败" >> "$FAILURE_REPORT"
                    ;;
                "RUNTIME_ERROR")
                    echo "**错误类型**: 运行时错误" >> "$FAILURE_REPORT"
                    ;;
            esac
            echo "" >> "$FAILURE_REPORT"
            
            # Pascal 源码
            if [ -f "$pas_file" ]; then
                echo "### Pascal 源码" >> "$FAILURE_REPORT"
                echo '```pascal' >> "$FAILURE_REPORT"
                cat "$pas_file" >> "$FAILURE_REPORT"
                echo '```' >> "$FAILURE_REPORT"
                echo "" >> "$FAILURE_REPORT"
            fi
            
            # 生成的 C 代码
            if [ -f "$c_file" ]; then
                echo "### 生成的 C 代码" >> "$FAILURE_REPORT"
                echo '```c' >> "$FAILURE_REPORT"
                cat "$c_file" >> "$FAILURE_REPORT"
                echo '```' >> "$FAILURE_REPORT"
                echo "" >> "$FAILURE_REPORT"
            fi
            
            # 错误信息
            if [ -f "$error_file" ] && [ -s "$error_file" ]; then
                echo "### 错误日志" >> "$FAILURE_REPORT"
                echo '```' >> "$FAILURE_REPORT"
                cat "$error_file" >> "$FAILURE_REPORT"
                echo '```' >> "$FAILURE_REPORT"
                echo "" >> "$FAILURE_REPORT"
            fi
            
            echo "---" >> "$FAILURE_REPORT"
            echo "" >> "$FAILURE_REPORT"
        fi
    fi
done

echo "共收集 $failed_count 个失败测试用例"
echo "详细报告已保存到：$FAILURE_REPORT"
