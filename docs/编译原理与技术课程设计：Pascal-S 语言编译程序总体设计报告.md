# 编译原理与技术课程设计：Pascal-S 语言编译程序总体设计报告

## 1. 引言

### 1.1 编写目的
本总体设计报告基于《Pascal-S 语言编译程序需求分析报告》，旨在将系统需求转化为软件总体架构与数据结构体系。本报告确立了编译程序的整体工作流程、模块划分、全局数据结构、核心符号表的设计方案以及各模块间的接口规范，为下一阶段的详细设计、团队并行编码以及单元测试提供全局指导与技术规约。

### 1.2 设计原则
1. **高内聚、低耦合**：将编译器划分为词法、语法、语义、代码生成及错误处理五大独立子模块，便于团队成员分工协作。
2. **语法制导翻译架构**：以语法分析器为驱动核心，在语法归约/推导的过程中，按需调用词法扫描器获取 Token，并触发语义动作和代码生成。
3. **健壮性与可扩展性**：设计全局的错误处理中心，实施“恐慌模式”等恢复策略；数据结构预留扩展字段，兼容后续向 ARM/RISC-V 汇编或 C 语言目标代码的生成。

---

## 2. 软件总体架构与工作流程

### 2.1 总体架构说明
本系统采用**单遍（Single-Pass）为主、语法树遍历为辅**的混合编译架构（视目标代码生成复杂度而定）。整个编译器以**语法分析器（Parser）** 为中心枢纽。

* **驱动层**：主控程序（Main）初始化全局数据与符号表后，调用语法分析器的 `Parse()` 方法启动编译。
* **协同层**：语法分析器在执行过程中，通过 `GetNextToken()` 接口按需从**词法分析器（Lexer）** 获取单词符号；在识别出特定的语法结构后，调用**语义分析器（Semantic Analyzer）** 进行类型检查并查填**符号表（Symbol Table）**。
* **输出层**：语义验证无误后，将中间结果移交**代码生成器（Code Generator）** 转换为目标代码。任何阶段发现异常，均向**全局错误处理模块（Error Handler）** 抛出异常信号。

### 2.2 模块间关系与数据流图

```text[源程序 source.pas] 
       │ (字符流)
       ▼
┌─────────────────┐  (Token流)  ┌─────────────────┐
│  词法分析模块   │ ◀───────── │  语法分析模块   │ (主控/驱动)
└─────────────────┘  (取Token)  └─────────────────┘
                                   │       │
             (查填属性、作用域)    │       │ (语法制导动作/AST)
             ┌─────────────────────┘       │
             ▼                             ▼
┌─────────────────┐             ┌─────────────────┐
│ 符号表管理模块  │             │ 语义与代码生成  │
└─────────────────┘             └─────────────────┘
             │                             │
             └──────────────┬──────────────┘
                            ▼
                   ┌─────────────────┐
                   │  错误处理模块   │ (输出错误报告并尝试恢复)
                   └─────────────────┘
                            │
                            ▼
                  [目标代码 (.c 或 .s)]
```

---

## 3. 功能模块划分与设计

### 3.1 词法分析模块 (Lexer)
* **模块功能**：通过 Flex 工具自动生成词法分析器，逐字符扫描源程序，跳过空白符与注释 `{...}`，利用有限状态机识别关键字、标识符、常数及界符等，并打包为标准 Token 返回。
* **核心接口**：`int yylex()`（由 Flex 自动生成）

### 3.2 语法分析模块 (Parser)
* **模块功能**：对 Token 流进行语法检查。采用自底向上的 LALR(1) 分析法，借助 Bison 工具自动生成分析器。
* **模块职责**：校验句子合法性；在匹配到特定产生式时，构建抽象语法树（AST）节点；遇到语法错误时启动错误恢复策略。
* **核心接口**：`int yyparse()`（由 Bison 自动生成）

### 3.3 语义分析与符号表模块 (Semantic Analyzer)
* **模块功能**：处理上下文相关的静态语义审查。包括标识符重复声明检查、未声明使用检查、赋值与运算类型相容性检查（Integer/Real/Boolean/Char）、数组越界检查及函数传参校验。
* **核心接口**：`void checkType(Type t1, Type t2)`, `void processDeclaration(...)`

### 3.4 代码生成模块 (Code Generator)
* **模块功能**：将合法的 AST 结构通过 Visitor 模式遍历，映射为等价的 C 语言源代码。生成的 .c 文件可直接用 GCC 编译执行。
* **核心接口**：`std::string CodeGenerator::generate(ProgramNode* program)`

### 3.5 错误处理模块 (Error Handler)
* **模块功能**：统一管理各阶段的警告与错误输出，格式化打印（行号、列号、错误类型），执行同步记号机制以避免编译器崩溃。
* **核心接口**：`void reportError(int line, int col, ErrorCode code, string msg)`

---

## 4. 全局数据结构设计

### 4.1 单词符号结构 (Token)
定义统一的结构体用于在词法与语法分析器之间传递数据：
```c
struct Token {
    TokenType type;    // 类别编码 (如 ID, NUM, ASSIGNOP, IF, WHILE等)
    string lexeme;     // 原始字符串 (如 "gcd", ":=")
    union {
        int i_val;     // 整型常数值
        float f_val;   // 实型常数值
    } value;
    int line_no;       // 行号 (用于定位错误)
    int col_no;        // 列号
};
```

### 4.2 语法树结构 (Abstract Syntax Tree)
系统采用 C++ 面向对象的设计，基于 **Visitor 访问者模式** 构建 AST 节点体系：
```cpp
// 基类：所有 AST 节点的抽象基类
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;  // 接受访问者
};

// 表达式节点：IntegerLiteralNode, RealLiteralNode, IdentifierNode,
//              BinaryExpressionNode, ArrayAccessNode, FunctionCallNode 等
// 语句节点：AssignmentNode, IfStatementNode, WhileStatementNode,
//              ForStatementNode, CompoundStatementNode, ProcedureCallNode 等
// 声明节点：VariableDeclarationNode, FunctionDeclarationNode
// 根节点：ProgramNode
```
每个节点类实现 `accept()` 方法，代码生成器作为 `ASTVisitor` 的具体实现，通过重载 `visit()` 方法对不同节点进行差异化处理。

---

## 5. 符号表设计 (核心设计)

根据《需求分析》对 Pascal-S 语言特性的约束：**复合语句允许嵌套，但过程和函数定义不允许嵌套**。这一特性决定了符号表的逻辑结构相对简单，只需维护**全局作用域**与**单一局部作用域**即可。

### 5.1 符号表内容与表项结构
符号表中存储标识符的各类属性信息，表项结构设计如下：
```c
enum IdKind { CONSTANT, VARIABLE, PROCEDURE, FUNCTION, ARRAY };
enum DataType { INT, REAL, BOOL, CHAR, NONE };

struct SymbolEntry {
    string name;       // 标识符名称
    IdKind kind;       // 标识符种类（常量/变量/数组/过程/函数）
    DataType type;     // 数据类型

    // 联合体：根据 kind 存储不同附加属性
    union {
        int const_val;             // 常量的值
        struct {                   
            int low_bound;         // 数组下界
            int high_bound;        // 数组上界
        } array_info;
        struct {                   
            int param_count;       // 函数/过程的参数个数
            bool is_ref[MAX_ARG];  // 各参数是否为引用传递 (传址)
            DataType arg_types[MAX_ARG]; // 各参数的数据类型
        } func_info;
    } attr;
    
    int level;         // 作用域层级 (0: 全局, 1: 局部)
    int memory_offset; // 在目标代码生成时分配的内存偏移量
};
```

### 5.2 符号表的逻辑结构
采用**栈式分块结构 (Block-structured Stack)** 或 **两级哈希表** 进行管理：
1. **全局表 (Global Table)**：位于栈底，记录程序体顶层定义的 const、var、procedure 和 function。生命周期贯穿整个编译过程。
2. **局部表 (Local Table)**：位于栈顶，由于子程序不可嵌套，同一时刻最多存在一个局部作用域。当进入某个过程/函数时创建，记录参数与局部变量。

### 5.3 管理程序接口操作
* **定位操作 `locate()`**：当解析到 `procedure` 或 `function` 头部时调用。标记全局表的当前位置，确立一个新的局部作用域起点。
* **插入操作 `insert(SymbolEntry entry)`**：向当前激活的层级（优先局部表，若无则入全局表）插入标识符。若同层级已存在同名标识符，则抛出“重复声明”错误。
* **查找操作 `lookup(string name)`**：按标识符名称查表。**遵循就近原则**：先在局部表中查找，若找不到再到全局表中查找。查不到则抛出“未声明标识符”错误。
* **重定位操作 `relocate()`**：当解析到子程序的 `end;` 时调用。将指针回退至 `locate()` 记录的起点，自动批量“删除（或失效）”局部于当前过程的所有名字，退出局部作用域。

---

## 6. 模块间接口定义与规约

### 6.1 Lexer 与 Parser 的接口
* 词法分析器由 Flex 自动生成，编译为 `lexer.cpp`。向 Parser 提供 `int yylex()` 函数，通过全局变量 `yylval` 传递 Token 属性值。
* 数据流通过 Bison 定义的 `%union` 结构体进行类型安全的隔离。

### 6.2 Parser 与 Symbol Table 的接口
* 当语法分析器推导出声明语句（如 `var a: integer;`）时，调用 `SymbolTable::insert("a", VARIABLE, INT, ...)`。
* 当推导出使用变量（如 `a := 1;`）时，调用 `SymbolTable::lookup("a")` 获取变量属性并检查。

### 6.3 Parser/Semantic 与 CodeGen 的接口
* Parser 构建完整的 AST 后，代码生成器通过 Visitor 模式遍历 AST 节点，对每种节点类型调用相应的 `visit()` 方法生成 C 代码。
* 对于函数传参：通过符号表中记录的 `is_reference` 属性，通知代码生成器生成值传递或指针传递的 C 代码。

---

## 7. 错误处理与恢复策略设计

针对可能出现的各类错误，设计完善的错误响应及自我恢复机制：

### 7.1 错误恢复方案 (恐慌模式 Panic Mode)
为了防止编译器在遇到首个语法错误后崩溃或产生大量的连锁误报，采用**恐慌模式**恢复策略：
1. 当 Parser 发现 Token 不匹配时，记录错误日志，进入“恐慌状态”。
2. 不断丢弃输入流中的后续 Token，直到遇到属于**同步词法单元集合 (Synchronizing Set)** 的 Token 为止。
3. 同步集合通常定义为：分号 `;`、保留字 `end`、`begin`、`var` 等语句级界符。
4. 遇到同步单元后，重置 Parser 状态，继续往下解析下一个完整的语句。

### 7.2 语义容错机制
若遇到表达式中变量未声明，符号表应抛出错误，但为了不影响后续代码的类型推导，符号表管理模块会在报错的同时，**自动隐式地向符号表中插入一个类型为 `NONE`或 `INT` 的虚拟标识符**。此举能避免同一个未声明变量在后续引发一连串的类型不匹配报错（报错雪崩）。