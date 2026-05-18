#set page(margin: (x: 2.4cm, y: 2.5cm))
#set text(font: ("SimSun", "Microsoft YaHei"), size: 11pt)
#set par(first-line-indent: 2em, leading: 0.8em)
#show heading.where(level: 1): it => [
  #v(1.0em)
  #set text(weight: "bold", size: 15pt)
  #it
  #v(0.5em)
]

#align(center)[
  #v(3.5cm)
  #text(20pt, weight: "bold")[Pascal-S 编译器程序使用说明]
  #v(0.8cm)
  #text(13pt)[北京邮电大学《编译原理与技术课程设计》]
  #v(2.0cm)
  #text(12pt)[项目名称：Pascal-S 到 C 的源到源编译器]
  #v(0.5cm)
  #text(12pt)[指导教师：王吴凡]
  #v(0.5cm)
  #text(12pt)[班级：304]
  #v(2.2cm)
  #text(12pt)[2026 年 5 月]
]

#pagebreak()

#outline(title: [目 录])

#pagebreak()

= 1．程序概述

本程序是一个面向 Pascal-S 语言子集的源到源编译器，其核心功能是将输入的 Pascal-S 源程序翻译为等价的 C 语言源程序。系统采用“词法分析 -> 语法分析与 AST 构建 -> 语义分析 -> C 代码生成”的流水线结构，既满足课程平台对自动评测的要求，也便于在本地环境中进行调试、验证和答辩展示。

当前版本支持常量、变量、数组、函数、过程、控制流、输入输出、布尔字面量、函数返回值处理、`record` 结构与多级字段访问等功能，并支持带源码定位的 `caret diagnostics` 错误提示。编译器的输出结果为 `.c` 文件，后续可使用 GCC 等 C 编译器继续编译并执行。

= 2．运行环境

== 2.1 开发与本地验证环境

建议在 Windows 环境下使用如下工具链进行构建与运行：

- 操作系统：Windows 10 / Windows 11
- C/C++ 编译器：GCC / G++（如 MinGW64）
- 词法分析工具：Flex
- 语法分析工具：Bison
- 构建工具：CMake 或 Makefile
- 命令行环境：PowerShell

本项目在本地开发时使用的典型工具包括 `g++`、`gcc`、`win_flex`、`win_bison`、`cmake` 和 PowerShell 脚本。若使用 Linux 或课程平台环境，则可直接使用系统内提供的 `flex`、`bison`、`g++` 和 `make`。

== 2.2 平台验收环境

对于头歌平台环境，程序需要将最终生成的编译器可执行文件命名为 `pascc`，并放置在指定目录中供评测脚本调用。平台测试时会自动把 `.pas` 文件传入编译器，要求编译器生成对应 `.c` 文件，并由平台进一步编译和比较程序输出结果。

= 3．目录结构说明

与程序使用最相关的目录如下：

#table(
  columns: 2,
  stroke: 0.8pt,
  inset: 6pt,
  [目录/文件], [作用],
  [`src/`], [编译器核心源码，包括词法分析、语法分析、AST、语义分析、类型系统、代码生成和错误处理],
  [`course_tests/`], [课程自建测试集，包括成功样例、错误样例及自动化测试脚本],
  [`user_tests/`], [本地回归测试样例，用于快速验证功能],
  [`submission/`], [验收材料，包括报告、分组表、验收登记表和程序使用说明],
  [`CMakeLists.txt` / `Makefile`], [项目构建入口],
)

= 4．构建方法

== 4.1 Windows 下使用 CMake 构建

若本机已配置 `gcc/g++`、`flex`、`bison` 和 `cmake`，可在项目根目录执行：

```powershell
cmake -S . -B build-mingw2 -G "MinGW Makefiles"
cmake --build build-mingw2 -j 4
```

构建完成后，可执行文件通常位于 `build-mingw2/` 目录下，Windows 环境中的编译器程序名为 `pascc.exe`。

== 4.2 Linux / 平台环境下使用 Makefile 构建

在 Linux 或课程平台环境中，可在项目根目录执行：

```bash
make clean
make BISON=bison FLEX=flex TARGET=pascc
```

构建成功后，项目根目录下会生成 `pascc` 可执行文件。若是头歌平台环境，需要将其复制到平台要求的 `bin/` 目录中。

= 5．使用方法

== 5.1 基本命令格式

编译器的基本调用方式如下：

```text
pascc -i input.pas
```

其中 `input.pas` 为待编译的 Pascal-S 源程序。若未显式指定输出文件，则系统会默认在同目录下生成同名 `.c` 文件。

== 5.2 Windows 示例

```powershell
.\build-mingw2\pascc.exe -i .\course_tests\success\record_nested_access.pas
```

执行后会自动生成：

```text
.\course_tests\success\record_nested_access.c
```

若希望显式指定输出文件路径，可使用 `-o` 参数：

```powershell
.\build-mingw2\pascc.exe -i input.pas -o output.c
```

== 5.3 生成代码后的运行方法

编译器本身只负责把 Pascal-S 程序翻译为 C 程序。若要继续验证运行结果，可使用 GCC 对生成的 C 文件进行编译。例如：

```powershell
gcc .\course_tests\success\record_nested_access.c -o .\course_tests\success\record_nested_access.exe
.\course_tests\success\record_nested_access.exe
```

对于需要输入数据的程序，可结合输入文件或命令行重定向方式完成运行验证。

= 6．输入与输出说明

== 6.1 输入文件

输入为符合 Pascal-S 子集语法的 `.pas` 文件。程序支持：

- 程序头与声明部分
- 常量、变量、数组定义
- 函数与过程定义
- 赋值、条件、循环等语句
- 输入输出语句
- `record` 结构与多级字段访问

== 6.2 输出文件

输出为等价的 `.c` 文件。该文件可进一步由标准 C 编译器编译并运行。当前生成代码默认面向 C11 风格，并已通过本地严格编译检查。

= 7．错误提示说明

当输入程序存在词法、语法或语义错误时，编译器不会生成错误结果，而是输出清晰的诊断信息。错误信息通常包括：

- 错误类别，例如 `Lexical Error`、`Syntax Error`、`Semantic Error`
- 出错位置的行号与列号
- 对应源码原文
- 位于出错位置下方的 `^~~~` 箭头标记

例如，当访问不存在的 `record` 字段时，系统会提示类似如下信息：

```text
[Semantic Error] line 7, col 9: Unknown record field: missing
    node.missing := 1;
        ^~~~~~~
```

这种 `caret diagnostics` 设计便于用户快速定位出错位置，也是本项目相较基础版本的重要改进之一。

= 8．测试与验证方法

本项目提供多层次的测试支撑：

- 平台评测：头歌平台公开与隐藏测试
- 课程自建测试：`course_tests/success` 与 `course_tests/errors`
- 本地回归测试：`user_tests/`
- 严格生成代码检查：`course_tests/run_strict_c_checks.ps1`

在 Windows 环境下，可使用如下命令运行课程自建测试：

```powershell
.\course_tests\run_course_tests.ps1 -CompilerPath .\build-mingw2\pascc.exe
```

若需要验证生成代码的规范性，可执行：

```powershell
.\course_tests\run_strict_c_checks.ps1 -CompilerPath .\build-mingw2\pascc.exe
```

= 9．平台提交注意事项

在头歌平台提交时，应特别注意以下事项：

1. 最终可执行文件名应为 `pascc`
2. 需放置在平台要求的目录下，例如 `/data/workspace/myshixun/bin`
3. 运行脚本所需目录如 `bin/error` 应提前创建
4. 平台只认 Linux 可执行文件，不能直接提交 Windows 下生成的 `pascc.exe`
5. 若平台环境已提供 `flex`、`bison`、`g++` 和 `make`，建议在平台上重新构建一次可执行文件

= 10．常见问题

== 10.1 无法找到 `flex` 或 `bison`

说明本地环境变量未正确配置。应确认对应工具已安装，并且在命令行中执行 `flex --version`、`bison --version` 可以正常输出版本号。

== 10.2 编译器可以生成 `.c`，但生成代码无法编译

建议先检查是否使用了最新版本的编译器程序，其次可运行严格 C11 检查脚本定位问题。若仅个别样例失败，可结合生成的 `.c` 文件和对应 Pascal-S 源程序逐项排查。

== 10.3 平台测试时提示找不到 `bin/error`

说明平台脚本使用了错误输出目录，但对应目录尚未创建。可先执行：

```bash
mkdir -p /data/workspace/myshixun/bin/error
```

再重新构建和测试。

= 11．说明

本说明文件与课程设计报告、测试用例、源程序和可运行程序共同构成最终提交材料的一部分。若后续继续迭代功能，可在保持本说明整体结构不变的前提下补充新的构建方法、测试脚本和示例命令。
