# 小组模块分工说明与学习路线

这份说明用于组内对齐，目标不是追究代码量，而是确保每位成员都能讲清自己负责模块的输入、输出和核心逻辑。

## 一、推荐模块划分

### 1. 词法分析与语法前端

对应文件：

- `src/lexer.l`
- `src/parser.y`
- `src/parser_state.h`
- `src/parser_state.cpp`
- `src/ast.h`
- `src/ast.cpp`

负责内容：

- token 识别
- 文法规则
- AST 构建
- 解析阶段上下文维护

需要讲清：

- Pascal-S 源码如何先变成 token，再变成 AST
- 为什么 parser 尽量只负责“建树”

### 2. 语义分析与类型系统

对应文件：

- `src/semantic_analyzer.h`
- `src/semantic_analyzer.cpp`
- `src/type_resolver.h`
- `src/type_resolver.cpp`
- `src/symbol_table.h`
- `src/symbol_table.cpp`

负责内容：

- 符号表维护
- 作用域管理
- 类型推断
- 语义错误检查

需要讲清：

- 标识符声明和使用如何绑定
- 类型错误、参数错误、record 字段错误如何发现

### 3. 代码生成

对应文件：

- `src/codegen.h`
- `src/codegen.cpp`

负责内容：

- AST 到 C 的翻译
- 数组、函数、过程、record 的 C 映射
- 参数传递与临时变量处理

需要讲清：

- 为什么 Pascal 数组访问不能直接照抄到 C
- record 为什么要映射成 struct

### 4. 主控流程、错误系统与测试

对应文件：

- `src/main.cpp`
- `src/error.h`
- `src/error.cpp`
- `course_tests/`
- `user_tests/`

负责内容：

- 编译器入口流程
- 错误报告
- 自建测试与验证

需要讲清：

- 主流程为什么是 parse -> semantic -> codegen
- caret diagnostics 如何帮助定位问题
- 自建测试为什么是课设加分项

## 二、推荐学习顺序

1. 先读 `src/main.cpp`，理解整个流程。
2. 再读 `src/ast.h`，理解 AST 节点有哪些。
3. 然后按顺序读：
   - `src/lexer.l`
   - `src/parser.y`
   - `src/semantic_analyzer.cpp`
   - `src/codegen.cpp`
4. 最后结合 `course_tests/` 看每个功能点对应哪些测试。

## 三、每位成员最少要会讲的内容

- 自己模块的输入是什么
- 自己模块的输出是什么
- 自己模块和上下游如何衔接
- 自己模块里最重要的两个设计点是什么
- 当前模块还有哪些可以继续优化的地方

## 四、汇报时的统一口径

- 不说“我只负责很小一部分”
- 不说“这个其实是某某一个人写的”
- 不说“我们组有人没做事”

统一说法应当是：

- “我们按模块协作完成”
- “本次由对应模块负责人介绍该部分设计”
- “整体由组长进行联调、整理和汇总”

## 五、建议的课堂展示分配

1. 组长：总体架构、阶段进展、平台结果、下一步计划
2. 词法分析负责人：词法与定位
3. 语义分析负责人：符号表与类型检查
4. 代码生成负责人：翻译与语言映射

这样安排的优点是：

- 和项目模块边界一致
- 每个人讲的内容比较集中
- 老师提问时也容易按模块追问
