# Pascal-S 编译器 - 迭代修复工作流程

## 📋 测试脚本说明

### 1. 运行全量回归测试

```bash
# 进入项目目录
cd /Users/a1234/.openclaw/workspace-user-proxy/pascal-s-compiler

# 确保编译器已编译
make

# 运行全量测试
./scripts/run_all_tests.sh
```

**输出**:
- `test_output/results/*.txt` - 每个测试的输出结果
- `test_output/errors/*.txt` - 每个测试的错误日志
- `test_output/report.json` - JSON 格式的测试报告

### 2. 收集失败用例详情

```bash
# 生成失败用例分析报告
./scripts/collect_failures.sh
```

**输出**: `test_output/failure_report.md` - 包含所有失败用例的：
- Pascal 源码
- 生成的 C 代码
- 错误日志

### 3. 迭代修复流程

```
┌─────────────────────────────────────────────────────┐
│  1. 运行全量测试 (run_all_tests.sh)                  │
│     ↓                                                │
│  2. 查看失败用例列表                                 │
│     ↓                                                │
│  3. 生成详细报告 (collect_failures.sh)               │
│     ↓                                                │
│  4. 分析失败原因，修改编译器源码                     │
│     ↓                                                │
│  5. 重新编译编译器 (make)                            │
│     ↓                                                │
│  6. 回到步骤 1，验证修复并检查回归                   │
│     ↓                                                │
│  7. 所有测试通过 → 完成！                            │
└─────────────────────────────────────────────────────┘
```

### 4. 测试单个用例

```bash
# 编译单个 Pascal 文件
make test-single FILE=open_set/00_main.pas

# 编译并运行单个测试
make run-test FILE=open_set/00_main.pas
```

## 📊 当前状态

| 测试平台 | 通过 | 总数 | 通过率 |
|----------|------|------|--------|
| 本地测试 | 70 | 70 | 100% |
| Web 平台 | 29 | 95 | 30.5% |

## 🔍 主要问题分类

1. **Segmentation fault** - 基本程序结构崩溃
2. **变量未声明** - 生成的 C 代码中变量未声明
3. **函数指针误用** - 函数名未正确调用
4. **缺少 stdbool.h** - boolean 类型支持
5. **缺少 read 函数** - Pascal read 语句应生成 scanf
6. **常量未定义** - const 声明未正确处理
7. **语法解析错误** - 特定语法结构解析失败

## ⚠️ 重要提示

- **每次修改后必须运行全量测试**，确保没有引入回归问题
- **优先修复影响面大的问题**（如变量声明、函数调用）
- **保留所有测试输出**，便于对比分析

## 📁 目录结构

```
pascal-s-compiler/
├── src/                    # 编译器源码
│   ├── parser.y           # Bison 语法定义
│   ├── lexer.l            # Flex 词法定义
│   ├── codegen.h          # 代码生成器
│   └── ...
├── open_set/              # 测试用例
│   ├── *.pas             # Pascal 源码
│   ├── *.in              # 输入文件
│   └── problem.txt       # 问题描述
├── test_output/           # 测试输出（自动生成）
│   ├── c_code/           # 生成的 C 代码
│   ├── executables/      # 编译后的可执行文件
│   ├── results/          # 运行结果
│   ├── errors/           # 错误日志
│   └── failure_report.md # 失败分析报告
├── scripts/               # 测试脚本
│   ├── run_all_tests.sh  # 全量测试
│   └── collect_failures.sh # 收集失败用例
└── Makefile              # 构建配置
```
