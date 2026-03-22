# Pascal-S 编译器架构设计文档

> **版本**: 1.0  
> **目标语言**: C++ (实现) → C99 (输出)  
> **核心工具**: Bison/Yacc + Flex/Lex  
> **最后更新**: 2026-03-22

---

## 1. 背景与目标

### 1.1 Pascal-S 语言特性概述

Pascal-S 是 Pascal 语言的教学子集，保留了 Pascal 的核心特性，同时简化了复杂性。本编译器针对的 Pascal-S 变体具有以下特征：

**基本结构**:
```pascal
program main;
const
  /* 常量定义 */
var
  /* 变量定义 */
function/procedure name(params): type;
  /* 函数/过程体 */
begin
  /* 主程序体 */
end.
```

**核心语言特性**:
- **数据类型**: `integer`, `real`, `boolean`, `char`
- **复合类型**: 一维数组 `array [low..high] of type`
- **控制结构**: `if-then-else`, `while-do`, `for-to-do`
- **子程序**: `function` (有返回值), `procedure` (无返回值)
- **参数传递**: 值传递 (默认), 引用传递 (`var` 关键字)
- **递归支持**: 函数/过程可递归调用

### 1.2 编译器目标

| 目标 | 描述 |
|------|------|
| **输入** | Pascal-S 源代码 (.pas 文件) |
| **输出** | 符合 C99 标准的 C 源代码 (.c 文件) |
| **实现语言** | C++ (利用面向对象特性) |
| **解析工具** | Bison (语法分析) + Flex (词法分析) |
| **运行依赖** | 仅 C99 标准库，无第三方依赖 |

### 1.3 非目标 (明确不支持的特性)

以下特性**不在本编译器支持范围内**，遇到时应报错：

| 特性 | 原因 |
|------|------|
| **过程/函数的嵌套定义** | 增加符号表管理复杂度，C99 不支持嵌套函数 |
| **记录类型 (record)** | 需要结构体映射，增加代码生成复杂度 |
| **指针类型** | C 指针语义复杂，易引入内存安全问题 |
| **文件 I/O** | 需要运行时库支持，超出纯 C99 范围 |
| **集合类型 (set)** | Pascal 特有类型，C 无直接对应 |
| **多维数组** | 本设计仅支持一维数组 (测试集中有多维，需扩展) |

---

## 2. 系统架构

### 2.1 整体编译流程图

```
┌─────────────────────────────────────────────────────────────────┐
│                      Pascal-S 源代码 (.pas)                       │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                        词法分析器 (Lexer)                         │
│                        (Flex 生成)                               │
│                    识别 Token 流                                  │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼ (Token 流)
┌─────────────────────────────────────────────────────────────────┐
│                       语法分析器 (Parser)                        │
│                       (Bison 生成)                               │
│                  构建抽象语法树 (AST)                              │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼ (AST)
┌─────────────────────────────────────────────────────────────────┐
│                      语义分析器 (Semantic Analyzer)               │
│                   - 符号表构建与管理                              │
│                   - 类型检查                                     │
│                   - 作用域验证                                   │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼ (带注释的 AST)
┌─────────────────────────────────────────────────────────────────┐
│                       代码生成器 (Code Generator)                │
│                 Pascal-S 构造 → C99 代码映射                      │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                       C99 源代码 (.c)                            │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 核心模块划分

| 模块 | 文件 | 职责 |
|------|------|------|
| **Lexer** | `lexer.l` (Flex) | 词法分析，生成 Token 流 |
| **Parser** | `parser.y` (Bison) | 语法分析，构建 AST |
| **AST Nodes** | `src/ast/*.hpp` | AST 节点类层次结构 |
| **Symbol Table** | `src/symbol_table.hpp` | 两级栈式符号表管理 |
| **Semantic Analyzer** | `src/semantic_analyzer.hpp` | 类型检查、作用域验证 |
| **Code Generator** | `src/code_generator.hpp` | AST → C99 代码生成 |
| **Error Handler** | `src/error_handler.hpp` | 错误收集与恐慌模式恢复 |
| **Main Driver** | `src/main.cpp` | 编译流程编排 |

### 2.3 模块间数据流

```
lexer.l ──Token──► parser.y ──AST──► semantic_analyzer ──AST*──► code_generator ──► C 代码
                      │                                        ▲
                      └──────────► symbol_table ───────────────┘
```

### 2.4 Bison/Yacc 文法文件设计

**parser.y 结构**:

```yacc
%{
// C++ 头文件包含
#include "ast/program_node.hpp"
#include "symbol_table.hpp"
#include "error_handler.hpp"
#include <string>
#include <vector>

// Bison 与 Flex 通信
extern int yylex();
extern void yyerror(const char* s);
extern int yylineno;

// 全局 AST 根节点
extern ProgramNode* root_ast;
%}

// 启用 C++ 解析器
%language "C++"
%define api.namespace {pascal_s}
%define api.parser.class {Parser}

// Token 定义 (与 lexer.l 同步)
%token <std::string> IDENTIFIER
%token <int> INTEGER_LITERAL
%token <double> REAL_LITERAL
%token <std::string> STRING_LITERAL
%token PROGRAM CONST VAR FUNCTION PROCEDURE
%token INTEGER REAL BOOLEAN CHAR ARRAY OF
%token BEGIN END IF THEN ELSE WHILE DO FOR TO DOWNTO
%token ASSIGN RELOP ADDOP MULOP NOT AND DIV MOD
%token LPAREN RPAREN LBRACKET RBRACKET SEMICOLON COMMA COLON DOT

// AST 节点指针类型
%type <ProgramNode*> program
%type <BlockNode*> block
%type <StatementNode*> statement
// ... 更多类型定义

// 错误恢复声明
%define parse.error verbose

%%

// 文法规则定义 (见第 5 节)

%%

// 错误处理函数
void pascal_s::Parser::error(const location_type& l, const std::string& m) {
    ErrorHandler::instance().syntax_error(l, m);
}
```

---

## 3. 核心数据结构定义 (C++ 风格)

### 3.1 Token 结构体/类

```cpp
// src/token.hpp
#pragma once
#include <string>
#include <variant>

namespace pascal_s {

enum class TokenType {
    // 关键字
    KW_PROGRAM, KW_CONST, KW_VAR, KW_FUNCTION, KW_PROCEDURE,
    KW_BEGIN, KW_END, KW_IF, KW_THEN, KW_ELSE, KW_WHILE, KW_DO,
    KW_FOR, KW_TO, KW_DOWNTO, KW_INTEGER, KW_REAL, KW_BOOLEAN,
    KW_CHAR, KW_ARRAY, KW_OF, KW_NOT, KW_AND, KW_DIV, KW_MOD,
    
    // 运算符
    OP_ASSIGN,     // :=
    OP_ADD, OP_SUB, OP_OR,
    OP_MUL, OP_DIV, OP_MOD, OP_AND,
    OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE,
    
    // 分隔符
    DELIM_LPAREN, DELIM_RPAREN,
    DELIM_LBRACKET, DELIM_RBRACKET,
    DELIM_SEMICOLON, DELIM_COMMA, DELIM_COLON, DELIM_DOT,
    
    // 字面量与标识符
    LIT_INTEGER, LIT_REAL, LIT_STRING, LIT_CHAR,
    TOK_IDENTIFIER,
    
    // 特殊
    TOK_EOF, TOK_ERROR
};

// Token 值 (使用 variant 存储不同类型)
using TokenValue = std::variant<
    std::monostate,      // 无值
    int,                 // 整数
    double,              // 实数
    std::string,         // 标识符/字符串
    char                 // 字符
>;

struct Token {
    TokenType type;
    TokenValue value;
    int line;
    int column;
    
    Token(TokenType t = TokenType::TOK_EOF, 
          TokenValue v = std::monostate{},
          int l = 0, int c = 0)
        : type(t), value(v), line(l), column(c) {}
    
    // 便捷访问
    int as_int() const { return std::get<int>(value); }
    double as_real() const { return std::get<double>(value); }
    const std::string& as_string() const { return std::get<std::string>(value); }
};

} // namespace pascal_s
```

### 3.2 SymbolEntry 类 (两级栈式符号表)

```cpp
// src/symbol_table.hpp
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

namespace pascal_s {

// 数据类型枚举
enum class DataType {
    INTEGER,
    REAL,
    BOOLEAN,
    CHAR,
    ARRAY,
    FUNCTION,
    PROCEDURE,
    VOID,
    UNKNOWN
};

// 数组类型信息
struct ArrayInfo {
    int lower_bound;
    int upper_bound;
    DataType element_type;
};

// 参数信息
struct ParameterInfo {
    std::string name;
    DataType type;
    bool is_reference;  // var 参数
    ArrayInfo array_info;
};

// 符号表条目
class SymbolEntry {
public:
    std::string name;
    DataType type;
    
    // 作用域信息
    int scope_level;
    int offset;  // 栈帧偏移
    
    // 额外信息 (根据类型)
    bool is_const;
    bool is_reference;  // 仅参数
    ArrayInfo array_info;
    std::vector<ParameterInfo> params;  // 仅函数/过程
    DataType return_type;  // 仅函数
    
    // 构造函数
    SymbolEntry(const std::string& n, DataType t, int level, int off = 0)
        : name(n), type(t), scope_level(level), offset(off),
          is_const(false), is_reference(false), return_type(DataType::VOID) {}
    
    // 是否为函数/过程
    bool is_subprogram() const {
        return type == DataType::FUNCTION || type == DataType::PROCEDURE;
    }
};

// 符号表 (两级栈式结构)
class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, std::shared_ptr<SymbolEntry>>> scopes;
    int current_offset;
    
public:
    SymbolTable() { enter_scope(); }  // 全局作用域
    
    // 作用域管理
    void enter_scope() {
        scopes.emplace_back();
        current_offset = 0;
    }
    
    void exit_scope() {
        if (scopes.size() > 1) {  // 保留全局作用域
            scopes.pop_back();
        }
    }
    
    int current_scope_level() const { return scopes.size() - 1; }
    
    // 符号插入与查找
    bool insert(const std::string& name, DataType type, 
                bool is_const = false, bool is_ref = false) {
        if (exists_in_current_scope(name)) return false;
        
        auto entry = std::make_shared<SymbolEntry>(
            name, type, current_scope_level(), current_offset++);
        entry->is_const = is_const;
        entry->is_reference = is_ref;
        scopes.back()[name] = entry;
        return true;
    }
    
    std::shared_ptr<SymbolEntry> lookup(const std::string& name) {
        for (int i = scopes.size() - 1; i >= 0; --i) {
            auto it = scopes[i].find(name);
            if (it != scopes[i].end()) return it->second;
        }
        return nullptr;
    }
    
    bool exists_in_current_scope(const std::string& name) {
        return scopes.back().find(name) != scopes.back().end();
    }
    
    // 函数/过程特殊处理
    void add_function(const std::string& name, DataType return_type,
                      const std::vector<ParameterInfo>& params) {
        auto entry = std::make_shared<SymbolEntry>(
            name, DataType::FUNCTION, 0, 0);
        entry->return_type = return_type;
        entry->params = params;
        scopes[0][name] = entry;  // 函数在全局作用域
    }
};

} // namespace pascal_s
```

### 3.3 AST 节点类层次结构 (C++ 多态设计)

```cpp
// src/ast/ast_node.hpp
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace pascal_s {

// 前向声明
class ExpressionNode;
class StatementNode;
class DeclarationNode;

// AST 节点基类 (使用 CRTP 模式)
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string node_type() const = 0;
    virtual void accept(class ASTVisitor& visitor) = 0;
    
    // 位置信息 (用于错误报告)
    int line = 0;
    int column = 0;
};

// ============ 表达式节点 ============

class ExpressionNode : public ASTNode {
public:
    virtual bool is_lvalue() const { return false; }
};

// 字面量
class IntegerLiteralNode : public ExpressionNode {
public:
    int value;
    IntegerLiteralNode(int v) : value(v) {}
    std::string node_type() const override { return "IntegerLiteral"; }
    void accept(ASTVisitor& visitor) override;
};

class RealLiteralNode : public ExpressionNode {
public:
    double value;
    RealLiteralNode(double v) : value(v) {}
    std::string node_type() const override { return "RealLiteral"; }
    void accept(ASTVisitor& visitor) override;
};

class CharLiteralNode : public ExpressionNode {
public:
    char value;
    CharLiteralNode(char v) : value(v) {}
    std::string node_type() const override { return "CharLiteral"; }
    void accept(ASTVisitor& visitor) override;
};

// 标识符 (变量/函数)
class IdentifierNode : public ExpressionNode {
public:
    std::string name;
    IdentifierNode(const std::string& n) : name(n) {}
    std::string node_type() const override { return "Identifier"; }
    void accept(ASTVisitor& visitor) override;
    bool is_lvalue() const override { return true; }
};

// 数组访问
class ArrayAccessNode : public ExpressionNode {
public:
    std::string array_name;
    std::unique_ptr<ExpressionNode> index;
    ArrayAccessNode(const std::string& name, std::unique_ptr<ExpressionNode> idx)
        : array_name(name), index(std::move(idx)) {}
    std::string node_type() const override { return "ArrayAccess"; }
    void accept(ASTVisitor& visitor) override;
    bool is_lvalue() const override { return true; }
};

// 二元运算
enum class BinaryOp { ADD, SUB, MUL, DIV, MOD, AND, OR, 
                      EQ, NE, LT, LE, GT, GE };

class BinaryExpressionNode : public ExpressionNode {
public:
    BinaryOp op;
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;
    
    BinaryExpressionNode(BinaryOp o, 
                         std::unique_ptr<ExpressionNode> l,
                         std::unique_ptr<ExpressionNode> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    
    std::string node_type() const override { return "BinaryExpression"; }
    void accept(ASTVisitor& visitor) override;
};

// 一元运算
enum class UnaryOp { NOT, NEGATE };

class UnaryExpressionNode : public ExpressionNode {
public:
    UnaryOp op;
    std::unique_ptr<ExpressionNode> operand;
    
    UnaryExpressionNode(UnaryOp o, std::unique_ptr<ExpressionNode> e)
        : op(o), operand(std::move(e)) {}
    
    std::string node_type() const override { return "UnaryExpression"; }
    void accept(ASTVisitor& visitor) override;
};

// 函数调用
class FunctionCallNode : public ExpressionNode {
public:
    std::string func_name;
    std::vector<std::unique_ptr<ExpressionNode>> arguments;
    
    FunctionCallNode(const std::string& name) : func_name(name) {}
    
    void add_argument(std::unique_ptr<ExpressionNode> arg) {
        arguments.push_back(std::move(arg));
    }
    
    std::string node_type() const override { return "FunctionCall"; }
    void accept(ASTVisitor& visitor) override;
};

// ============ 语句节点 ============

class StatementNode : public ASTNode {};

// 赋值语句
class AssignmentNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> target;  // 变量或数组访问
    std::unique_ptr<ExpressionNode> value;
    
    AssignmentNode(std::unique_ptr<ExpressionNode> t,
                   std::unique_ptr<ExpressionNode> v)
        : target(std::move(t)), value(std::move(v)) {}
    
    std::string node_type() const override { return "Assignment"; }
    void accept(ASTVisitor& visitor) override;
};

// 复合语句 (begin...end)
class CompoundStatementNode : public StatementNode {
public:
    std::vector<std::unique_ptr<StatementNode>> statements;
    
    void add_statement(std::unique_ptr<StatementNode> stmt) {
        statements.push_back(std::move(stmt));
    }
    
    std::string node_type() const override { return "CompoundStatement"; }
    void accept(ASTVisitor& visitor) override;
};

// if 语句
class IfStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<StatementNode> then_branch;
    std::unique_ptr<StatementNode> else_branch;  // 可为空
    
    IfStatementNode(std::unique_ptr<ExpressionNode> cond,
                    std::unique_ptr<StatementNode> then_stmt)
        : condition(std::move(cond)), then_branch(std::move(then_stmt)) {}
    
    void set_else_branch(std::unique_ptr<StatementNode> else_stmt) {
        else_branch = std::move(else_stmt);
    }
    
    std::string node_type() const override { return "IfStatement"; }
    void accept(ASTVisitor& visitor) override;
};

// while 循环
class WhileStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<StatementNode> body;
    
    WhileStatementNode(std::unique_ptr<ExpressionNode> cond,
                       std::unique_ptr<StatementNode> b)
        : condition(std::move(cond)), body(std::move(b)) {}
    
    std::string node_type() const override { return "WhileStatement"; }
    void accept(ASTVisitor& visitor) override;
};

// for 循环
class ForStatementNode : public StatementNode {
public:
    std::string loop_var;
    std::unique_ptr<ExpressionNode> start;
    std::unique_ptr<ExpressionNode> end;
    std::unique_ptr<StatementNode> body;
    bool is_downto;  // true = downto, false = to
    
    ForStatementNode(const std::string& var,
                     std::unique_ptr<ExpressionNode> s,
                     std::unique_ptr<ExpressionNode> e,
                     std::unique_ptr<StatementNode> b,
                     bool down = false)
        : loop_var(var), start(std::move(s)), end(std::move(e)),
          body(std::move(b)), is_downto(down) {}
    
    std::string node_type() const override { return "ForStatement"; }
    void accept(ASTVisitor& visitor) override;
};

// 过程调用语句
class ProcedureCallNode : public StatementNode {
public:
    std::string proc_name;
    std::vector<std::unique_ptr<ExpressionNode>> arguments;
    
    ProcedureCallNode(const std::string& name) : proc_name(name) {}
    
    void add_argument(std::unique_ptr<ExpressionNode> arg) {
        arguments.push_back(std::move(arg));
    }
    
    std::string node_type() const override { return "ProcedureCall"; }
    void accept(ASTVisitor& visitor) override;
};

// 写语句
class WriteStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> value;
    
    WriteStatementNode(std::unique_ptr<ExpressionNode> v)
        : value(std::move(v)) {}
    
    std::string node_type() const override { return "WriteStatement"; }
    void accept(ASTVisitor& visitor) override;
};

// ============ 声明节点 ============

class DeclarationNode : public ASTNode {};

// 变量声明
class VariableDeclarationNode : public DeclarationNode {
public:
    std::string var_name;
    DataType type;
    bool is_array;
    ArrayInfo array_info;
    bool is_const;
    std::unique_ptr<ExpressionNode> init_value;  // 常量初始值
    
    VariableDeclarationNode(const std::string& name, DataType t)
        : var_name(name), type(t), is_array(false), is_const(false) {}
    
    std::string node_type() const override { return "VariableDeclaration"; }
    void accept(ASTVisitor& visitor) override;
};

// 函数/过程声明
class FunctionDeclarationNode : public DeclarationNode {
public:
    std::string func_name;
    DataType return_type;
    std::vector<ParameterInfo> parameters;
    std::unique_ptr<CompoundStatementNode> body;
    bool is_procedure;  // true = procedure, false = function
    
    FunctionDeclarationNode(const std::string& name, bool is_proc = false)
        : func_name(name), return_type(DataType::VOID), is_procedure(is_proc) {}
    
    std::string node_type() const override { return "FunctionDeclaration"; }
    void accept(ASTVisitor& visitor) override;
};

// ============ 程序根节点 ============

class ProgramNode : public ASTNode {
public:
    std::string program_name;
    std::vector<std::unique_ptr<DeclarationNode>> declarations;
    std::unique_ptr<CompoundStatementNode> main_body;
    
    ProgramNode(const std::string& name) : program_name(name) {}
    
    void add_declaration(std::unique_ptr<DeclarationNode> decl) {
        declarations.push_back(std::move(decl));
    }
    
    std::string node_type() const override { return "Program"; }
    void accept(ASTVisitor& visitor) override;
};

// ============ 访问者模式 ============

class ASTVisitor {
public:
    virtual void visit(IntegerLiteralNode& n) = 0;
    virtual void visit(RealLiteralNode& n) = 0;
    virtual void visit(IdentifierNode& n) = 0;
    virtual void visit(BinaryExpressionNode& n) = 0;
    virtual void visit(AssignmentNode& n) = 0;
    virtual void visit(IfStatementNode& n) = 0;
    virtual void visit(WhileStatementNode& n) = 0;
    virtual void visit(ForStatementNode& n) = 0;
    virtual void visit(FunctionDeclarationNode& n) = 0;
    virtual void visit(ProgramNode& n) = 0;
    // ... 更多 visit 方法
};

} // namespace pascal_s
```

### 3.4 类型系统定义

```cpp
// src/type_system.hpp
#pragma once
#include "symbol_table.hpp"
#include <unordered_map>

namespace pascal_s {

class TypeSystem {
public:
    // 类型兼容性检查
    static bool is_compatible(DataType from, DataType to) {
        if (from == to) return true;
        // integer 可隐式转换为 real
        if (from == DataType::INTEGER && to == DataType::REAL) return true;
        return false;
    }
    
    // 二元运算结果类型推断
    static DataType infer_binary_result(BinaryOp op, DataType left, DataType right) {
        // 关系运算返回 boolean
        if (op >= BinaryOp::EQ && op <= BinaryOp::GE) {
            return DataType::BOOLEAN;
        }
        // 算术运算
        if (left == DataType::REAL || right == DataType::REAL) {
            return DataType::REAL;
        }
        // div/mod 返回 integer
        if (op == BinaryOp::DIV || op == BinaryOp::MOD) {
            return DataType::INTEGER;
        }
        // 默认 integer
        return DataType::INTEGER;
    }
    
    // 类型名称 (用于代码生成)
    static std::string to_c_type(DataType t) {
        switch (t) {
            case DataType::INTEGER: return "int";
            case DataType::REAL: return "double";
            case DataType::BOOLEAN: return "int";  // C 无原生 bool (C99)
            case DataType::CHAR: return "char";
            default: return "void";
        }
    }
};

} // namespace pascal_s
```

---

## 4. 接口定义

### 4.1 各模块公共 API

```cpp
// src/compiler.hpp - 编译器主接口
#pragma once
#include <string>
#include <memory>

namespace pascal_s {

class Compiler {
public:
    static Compiler& instance();
    
    // 编译入口
    bool compile(const std::string& input_file, 
                 const std::string& output_file);
    
    // 编译 Pascal-S 源码字符串
    bool compile_string(const std::string& source,
                        std::string& output);
    
    // 获取错误信息
    const std::vector<std::string>& get_errors() const;
    bool has_errors() const;
    
private:
    Compiler() = default;
    
    std::unique_ptr<class Lexer> lexer;
    std::unique_ptr<class Parser> parser;
    std::unique_ptr<class SemanticAnalyzer> semantic_analyzer;
    std::unique_ptr<class CodeGenerator> code_generator;
    std::vector<std::string> errors;
};

} // namespace pascal_s
```

```cpp
// src/error_handler.hpp - 错误处理器
#pragma once
#include <string>
#include <vector>

namespace pascal_s {

enum class ErrorType {
    LEXICAL,
    SYNTAX,
    SEMANTIC,
    CODE_GENERATION
};

struct CompilerError {
    ErrorType type;
    std::string message;
    int line;
    int column;
    bool recovered;  // 是否已恢复
};

class ErrorHandler {
public:
    static ErrorHandler& instance();
    
    void lexical_error(const std::string& msg, int line, int col);
    void syntax_error(const std::string& msg, int line, int col);
    void semantic_error(const std::string& msg, int line, int col);
    
    // 恐慌模式恢复
    void panic_mode();
    void synchronize();
    
    // 错误查询
    const std::vector<CompilerError>& get_errors() const;
    bool has_critical_errors() const;
    void clear();
    
private:
    ErrorHandler() = default;
    std::vector<CompilerError> errors;
    bool in_panic_mode = false;
};

} // namespace pascal_s
```

### 4.2 Lex/Bison 之间的 Token 通信规约

**lexer.l 与 parser.y 的 Token 同步**:

```lex
/* lexer.l - 词法规则片段 */
%{
#include "parser.tab.hpp"  // Bison 生成的头文件
#include "token.hpp"
extern int yylineno;
%}

%%

/* 关键字 */
"program"    { return pascal_s::Parser::make_PROGRAM(yylineno); }
"const"      { return pascal_s::Parser::make_CONST(yylineno); }
"var"        { return pascal_s::Parser::make_VAR(yylineno); }
"function"   { return pascal_s::Parser::make_FUNCTION(yylineno); }
"procedure"  { return pascal_s::Parser::make_PROCEDURE(yylineno); }
"begin"      { return pascal_s::Parser::make_BEGIN(yylineno); }
"end"        { return pascal_s::Parser::make_END(yylineno); }
"if"         { return pascal_s::Parser::make_IF(yylineno); }
"then"       { return pascal_s::Parser::make_THEN(yylineno); }
"else"       { return pascal_s::Parser::make_ELSE(yylineno); }
"while"      { return pascal_s::Parser::make_WHILE(yylineno); }
"do"         { return pascal_s::Parser::make_DO(yylineno); }
"for"        { return pascal_s::Parser::make_FOR(yylineno); }
"to"         { return pascal_s::Parser::make_TO(yylineno); }
"downto"     { return pascal_s::Parser::make_DOWNTO(yylineno); }
"integer"    { return pascal_s::Parser::make_INTEGER(yylineno); }
"real"       { return pascal_s::Parser::make_REAL(yylineno); }
"boolean"    { return pascal_s::Parser::make_BOOLEAN(yylineno); }
"char"       { return pascal_s::Parser::make_CHAR(yylineno); }
"array"      { return pascal_s::Parser::make_ARRAY(yylineno); }
"of"         { return pascal_s::Parser::make_OF(yylineno); }
"not"        { return pascal_s::Parser::make_NOT(yylineno); }
"and"        { return pascal_s::Parser::make_AND(yylineno); }
"div"        { return pascal_s::Parser::make_DIV(yylineno); }
"mod"        { return pascal_s::Parser::make_MOD(yylineno); }

/* 运算符 */
":="         { return pascal_s::Parser::make_ASSIGN(yylineno); }
"+"          { return pascal_s::Parser::make_ADDOP(yylineno); }
"-"          { return pascal_s::Parser::make_ADDOP(yylineno); }
"*"          { return pascal_s::Parser::make_MULOP(yylineno); }
"/"          { return pascal_s::Parser::make_MULOP(yylineno); }
"="          { return pascal_s::Parser::make_RELOP(yylineno); }
"<>"         { return pascal_s::Parser::make_RELOP(yylineno); }
"<"          { return pascal_s::Parser::make_RELOP(yylineno); }
"<="         { return pascal_s::Parser::make_RELOP(yylineno); }
">"          { return pascal_s::Parser::make_RELOP(yylineno); }
">="         { return pascal_s::Parser::make_RELOP(yylineno); }

/* 标识符 */
[a-zA-Z_][a-zA-Z0-9_]* {
    yylval->as<std::string>() = yytext;
    return pascal_s::Parser::make_IDENTIFIER(yylineno);
}

/* 整数 */
[0-9]+ {
    yylval->as<int>() = std::stoi(yytext);
    return pascal_s::Parser::make_INTEGER_LITERAL(yylineno);
}

/* 跳过空白 */
[ \t\n]+ { /* ignore */ }

/* 注释 */
\{[^}]*\} | \(\*.*?\*\) { /* ignore comments */ }

%%
```

### 4.3 C++ 类接口设计

```cpp
// src/code_generator.hpp - 代码生成器接口
#pragma once
#include "ast/ast_node.hpp"
#include <string>
#include <sstream>

namespace pascal_s {

class CodeGenerator : public ASTVisitor {
public:
    CodeGenerator();
    
    // 生成 C 代码
    std::string generate(ProgramNode* program);
    
    // 访问者方法 (实现 AST 遍历)
    void visit(ProgramNode& n) override;
    void visit(FunctionDeclarationNode& n) override;
    void visit(AssignmentNode& n) override;
    void visit(IfStatementNode& n) override;
    void visit(WhileStatementNode& n) override;
    void visit(ForStatementNode& n) override;
    void visit(BinaryExpressionNode& n) override;
    void visit(IdentifierNode& n) override;
    // ... 更多 visit 方法
    
private:
    std::ostringstream output;
    int indent_level = 0;
    
    void indent();
    std::string c_operator(BinaryOp op);
    std::string c_type(DataType t);
};

} // namespace pascal_s
```

---

## 5. Pascal-S 语法子集定义

### 5.1 支持的语句类型

| 语句类型 | Pascal-S 语法 | 示例 |
|----------|--------------|------|
| 赋值 | `variable := expression` | `a := b + 1` |
| 复合语句 | `begin stmt; ... end` | `begin a:=1; b:=2 end` |
| if 语句 | `if cond then stmt [else stmt]` | `if x>0 then a:=1 else a:=0` |
| while 循环 | `while cond do stmt` | `while i<10 do i:=i+1` |
| for 循环 | `for var := start to/downto end do stmt` | `for i:=1 to 10 do write(i)` |
| 过程调用 | `proc_name(args)` | `swap(a, b)` |
| 函数调用 (表达式中) | `func_name(args)` | `x := max(a, b)` |
| 写语句 | `write(expression)` | `write(a + b)` |

### 5.2 支持的表达式

| 表达式类型 | 运算符 | 优先级 |
|------------|--------|--------|
| 字面量 | `123`, `3.14`, `'a'`, `true` | 最高 |
| 标识符 | `variable`, `array[i]` | - |
| 函数调用 | `func(args)` | - |
| 一元运算 | `not`, `-` (负号) | 高 |
| 乘除运算 | `*`, `/`, `div`, `mod`, `and` | 中 |
| 加减运算 | `+`, `-`, `or` | 低 |
| 关系运算 | `=`, `<>`, `<`, `<=`, `>`, `>=` | 最低 |

### 5.3 支持的数据类型

| 类型 | Pascal-S | C99 映射 | 说明 |
|------|----------|---------|------|
| 整数 | `integer` | `int` | 32 位有符号整数 |
| 实数 | `real` | `double` | 双精度浮点数 |
| 布尔 | `boolean` | `int` | 0=false, 非 0=true |
| 字符 | `char` | `char` | 单字符 |
| 数组 | `array [low..high] of T` | `T arr[size]` | 一维数组 |

### 5.4 Bison 文法规则设计

```yacc
/* parser.y - 核心文法规则 */

%start program

%%

/* 程序结构 */
program:
    PROGRAM IDENTIFIER SEMICOLON
    block
    DOT
    {
        $$ = new ProgramNode($2);
        $$->main_body = std::unique_ptr<CompoundStatementNode>($4);
        root_ast = $$;
    }
    ;

block:
    declaration_section
    BEGIN compound_statement END
    {
        $$ = $4;
    }
    | BEGIN compound_statement END
    {
        $$ = $2;
    }
    ;

/* 声明部分 */
declaration_section:
    const_declarations
    var_declarations
    subprogram_declarations
    {
        // 合并所有声明
    }
    | const_declarations
    | var_declarations
    | subprogram_declarations
    | %empty
    ;

const_declarations:
    CONST const_declaration_list
    | %empty
    ;

const_declaration_list:
    const_declaration_list IDENTIFIER EQ constant SEMICOLON
    {
        auto decl = new VariableDeclarationNode($2, infer_type($4));
        decl->is_const = true;
        decl->init_value = std::move($4);
        // 添加到 AST
    }
    | IDENTIFIER EQ constant SEMICOLON
    {
        auto decl = new VariableDeclarationNode($2, infer_type($4));
        decl->is_const = true;
        decl->init_value = std::move($4);
    }
    ;

var_declarations:
    VAR var_declaration_list
    | %empty
    ;

var_declaration_list:
    var_declaration_list IDENTIFIER COLON type_declaration SEMICOLON
    {
        auto decl = new VariableDeclarationNode($2, $4);
        // 添加到 AST
    }
    | IDENTIFIER COLON type_declaration SEMICOLON
    {
        auto decl = new VariableDeclarationNode($2, $4);
    }
    ;

type_declaration:
    INTEGER    { $$ = DataType::INTEGER; }
    | REAL     { $$ = DataType::REAL; }
    | BOOLEAN  { $$ = DataType::BOOLEAN; }
    | CHAR     { $$ = DataType::CHAR; }
    | ARRAY LBRACKET constant DOTDOT constant RBRACKET OF type_declaration
    {
        $$ = DataType::ARRAY;
        // 设置数组信息
    }
    ;

/* 子程序声明 */
subprogram_declarations:
    subprogram_declarations subprogram_declaration
    | subprogram_declaration
    ;

subprogram_declaration:
    function_header block SEMICOLON
    {
        auto func = $1;
        func->body = std::unique_ptr<CompoundStatementNode>($2);
        // 添加到 AST
    }
    ;

function_header:
    FUNCTION IDENTIFIER formal_parameters COLON type_declaration SEMICOLON
    {
        $$ = new FunctionDeclarationNode($2, false);
        $$->return_type = $5;
        $$->parameters = $3;
    }
    | FUNCTION IDENTIFIER SEMICOLON
    {
        $$ = new FunctionDeclarationNode($2, false);
        $$->return_type = DataType::INTEGER;  // 默认返回类型
    }
    | PROCEDURE IDENTIFIER formal_parameters SEMICOLON
    {
        $$ = new FunctionDeclarationNode($2, true);
        $$->parameters = $3;
    }
    ;

formal_parameters:
    LPAREN parameter_list RPAREN
    {
        $$ = $2;
    }
    | %empty
    {
        $$ = std::vector<ParameterInfo>();
    }
    ;

parameter_list:
    parameter_list SEMICOLON parameter_group
    {
        $$ = $1;
        $$.insert($$.end(), $3.begin(), $3.end());
    }
    | parameter_group
    ;

parameter_group:
    VAR IDENTIFIER COLON type_declaration
    {
        ParameterInfo p;
        p.name = $2;
        p.type = $4;
        p.is_reference = true;
        $$ = std::vector<ParameterInfo>{p};
    }
    | IDENTIFIER COLON type_declaration
    {
        ParameterInfo p;
        p.name = $1;
        p.type = $3;
        p.is_reference = false;
        $$ = std::vector<ParameterInfo>{p};
    }
    ;

/* 语句 */
compound_statement:
    BEGIN statement_list END
    {
        $$ = new CompoundStatementNode();
        for (auto& stmt : $2) {
            $$->add_statement(std::move(stmt));
        }
    }
    ;

statement_list:
    statement_list SEMICOLON statement
    {
        $$ = $1;
        $$.push_back(std::move($3));
    }
    | statement
    {
        $$ = std::vector<std::unique_ptr<StatementNode>>();
        $$.push_back(std::move($1));
    }
    ;

statement:
    variable ASSIGN expression
    {
        $$ = std::make_unique<AssignmentNode>(std::move($1), std::move($3));
    }
    | compound_statement
    | if_statement
    | while_statement
    | for_statement
    | procedure_call
    | write_statement
    ;

if_statement:
    IF expression THEN statement
    {
        $$ = std::make_unique<IfStatementNode>(std::move($2), std::move($4));
    }
    | IF expression THEN statement ELSE statement
    {
        auto stmt = std::make_unique<IfStatementNode>(std::move($2), std::move($4));
        stmt->set_else_branch(std::move($6));
        $$ = std::move(stmt);
    }
    ;

while_statement:
    WHILE expression DO statement
    {
        $$ = std::make_unique<WhileStatementNode>(std::move($2), std::move($4));
    }
    ;

for_statement:
    FOR IDENTIFIER ASSIGN expression TO expression DO statement
    {
        $$ = std::make_unique<ForStatementNode>($2, std::move($4), std::move($6), std::move($8), false);
    }
    | FOR IDENTIFIER ASSIGN expression DOWNTO expression DO statement
    {
        $$ = std::make_unique<ForStatementNode>($2, std::move($4), std::move($6), std::move($8), true);
    }
    ;

procedure_call:
    IDENTIFIER actual_parameters
    {
        auto call = std::make_unique<ProcedureCallNode>($1);
        for (auto& arg : $2) {
            call->add_argument(std::move(arg));
        }
        $$ = std::move(call);
    }
    ;

write_statement:
    WRITE LPAREN expression RPAREN
    {
        $$ = std::make_unique<WriteStatementNode>(std::move($3));
    }
    ;

/* 表达式 */
expression:
    simple_expression
    | simple_expression RELOP simple_expression
    {
        $$ = std::make_unique<BinaryExpressionNode>(relop_to_binop($2), std::move($1), std::move($3));
    }
    ;

simple_expression:
    term
    | ADDOP term
    {
        $$ = std::make_unique<UnaryExpressionNode>(UnaryOp::NEGATE, std::move($2));
    }
    | simple_expression ADDOP term
    {
        $$ = std::make_unique<BinaryExpressionNode>(addop_to_binop($2), std::move($1), std::move($3));
    }
    | simple_expression OR term
    {
        $$ = std::make_unique<BinaryExpressionNode>(BinaryOp::OR, std::move($1), std::move($3));
    }
    ;

term:
    factor
    | term MULOP factor
    {
        $$ = std::make_unique<BinaryExpressionNode>(mulop_to_binop($2), std::move($1), std::move($3));
    }
    | term DIV factor
    {
        $$ = std::make_unique<BinaryExpressionNode>(BinaryOp::DIV, std::move($1), std::move($3));
    }
    | term MOD factor
    {
        $$ = std::make_unique<BinaryExpressionNode>(BinaryOp::MOD, std::move($1), std::move($3));
    }
    | term AND factor
    {
        $$ = std::make_unique<BinaryExpressionNode>(BinaryOp::AND, std::move($1), std::move($3));
    }
    ;

factor:
    INTEGER_LITERAL
    {
        $$ = std::make_unique<IntegerLiteralNode>($1);
    }
    | REAL_LITERAL
    {
        $$ = std::make_unique<RealLiteralNode>($1);
    }
    | IDENTIFIER
    {
        $$ = std::make_unique<IdentifierNode>($1);
    }
    | IDENTIFIER actual_parameters
    {
        auto call = std::make_unique<FunctionCallNode>($1);
        for (auto& arg : $2) {
            call->add_argument(std::move(arg));
        }
        $$ = std::move(call);
    }
    | IDENTIFIER LBRACKET expression RBRACKET
    {
        $$ = std::make_unique<ArrayAccessNode>($1, std::move($3));
    }
    | LPAREN expression RPAREN
    {
        $$ = std::move($2);
    }
    | NOT factor
    {
        $$ = std::make_unique<UnaryExpressionNode>(UnaryOp::NOT, std::move($2));
    }
    ;

actual_parameters:
    LPAREN expression_list RPAREN
    {
        $$ = std::move($2);
    }
    | %empty
    {
        $$ = std::vector<std::unique_ptr<ExpressionNode>>();
    }
    ;

expression_list:
    expression_list COMMA expression
    {
        $$ = $1;
        $$.push_back(std::move($3));
    }
    | expression
    {
        $$ = std::vector<std::unique_ptr<ExpressionNode>>();
        $$.push_back(std::move($1));
    }
    ;

variable:
    IDENTIFIER
    {
        $$ = std::make_unique<IdentifierNode>($1);
    }
    | IDENTIFIER LBRACKET expression RBRACKET
    {
        $$ = std::make_unique<ArrayAccessNode>($1, std::move($3));
    }
    ;

constant:
    INTEGER_LITERAL
    | REAL_LITERAL
    | CHAR_LITERAL
    ;

%%
```

---

## 6. 代码生成策略

### 6.1 Pascal-S 构造到 C99 的映射规则

| Pascal-S 构造 | C99 映射 | 说明 |
|--------------|---------|------|
| `program name` | `int main()` | 主程序转换为 main 函数 |
| `var x: integer` | `int x;` | 变量声明 |
| `const C = 10` | `#define C 10` 或 `const int C = 10` | 常量定义 |
| `begin ... end` | `{ ... }` | 复合语句块 |
| `:=` | `=` | 赋值运算符 |
| `and` | `&&` | 逻辑与 |
| `or` | `||` | 逻辑或 |
| `not` | `!` | 逻辑非 |
| `div` | `/` (整数) | 整数除法 |
| `mod` | `%` | 取模 |
| `write(x)` | `printf("%d", x)` | 输出 (需类型适配) |

### 6.2 变量声明转换

```cpp
// Pascal-S
var
  a: integer;
  b: real;
  c: boolean;
  d: char;
  arr: array[0..9] of integer;

// 生成的 C99 代码
int a;
double b;
int c;  // boolean 用 int 表示
char d;
int arr[10];  // array[0..9] → size=10
```

**代码生成器实现**:

```cpp
void CodeGenerator::visit(VariableDeclarationNode& n) {
    indent();
    if (n.is_array) {
        int size = n.array_info.upper_bound - n.array_info.lower_bound + 1;
        output << c_type(n.type) << " " << n.var_name 
               << "[" << size << "];\n";
    } else {
        output << c_type(n.type) << " " << n.var_name << ";\n";
    }
}
```

### 6.3 控制流转换

#### if 语句

```cpp
// Pascal-S
if (a > b) then
    max := a
else
    max := b;

// C99
if (a > b) {
    max = a;
} else {
    max = b;
}
```

#### while 循环

```cpp
// Pascal-S
while (i < 10) do
    i := i + 1;

// C99
while (i < 10) {
    i = i + 1;
}
```

#### for 循环

```cpp
// Pascal-S (to)
for i := 1 to 10 do
    sum := sum + i;

// C99
for (int i = 1; i <= 10; i++) {
    sum = sum + i;
}

// Pascal-S (downto)
for i := 10 downto 1 do
    sum := sum + i;

// C99
for (int i = 10; i >= 1; i--) {
    sum = sum + i;
}
```

**代码生成器实现**:

```cpp
void CodeGenerator::visit(ForStatementNode& n) {
    indent();
    output << "for (" << c_type(DataType::INTEGER) << " " 
           << n.loop_var << " = ";
    n.start->accept(*this);
    output << "; " << n.loop_var << (n.is_downto ? " >= " : " <= ");
    n.end->accept(*this);
    output << "; " << n.loop_var << (n.is_downto ? "--" : "++") << ") {\n";
    
    indent_level++;
    n.body->accept(*this);
    indent_level--;
    
    indent();
    output << "}\n";
}
```

### 6.4 过程/函数调用转换 (含 is_ref/var 参数处理)

```cpp
// Pascal-S
function add(a: integer; var b: integer): integer;
begin
    b := b + 1;
    add := a + b;
end;

// C99 (引用参数用指针)
int add(int a, int* b) {
    (*b) = (*b) + 1;
    return a + (*b);
}

// 调用
// Pascal-S: result := add(x, y);
// C99: result = add(x, &y);
```

**代码生成器实现**:

```cpp
void CodeGenerator::visit(FunctionDeclarationNode& n) {
    // 函数签名
    output << c_type(n.return_type) << " " << n.func_name << "(";
    
    bool first = true;
    for (const auto& param : n.parameters) {
        if (!first) output << ", ";
        first = false;
        
        if (param.is_reference) {
            output << c_type(param.type) << "* " << param.name;
        } else {
            output << c_type(param.type) << " " << param.name;
        }
    }
    
    output << ") {\n";
    
    // 函数体
    indent_level++;
    n.body->accept(*this);
    indent_level--;
    
    output << "}\n\n";
}

void CodeGenerator::visit(FunctionCallNode& n) {
    output << n.func_name << "(";
    
    // 需要根据符号表判断参数是否为引用类型
    // 这里简化处理，实际需查询 SymbolTable
    bool first = true;
    for (const auto& arg : n.arguments) {
        if (!first) output << ", ";
        first = false;
        
        // 如果是引用参数，需要取地址
        // if (is_reference_param(...)) output << "&";
        arg->accept(*this);
    }
    
    output << ")";
}
```

### 6.5 数组访问转换

```cpp
// Pascal-S
arr[i] := arr[j] + 1;

// C99
arr[i] = arr[j] + 1;

// Pascal-S (带偏移)
// array[0..9] → C 中直接使用 [0..9]
// array[1..10] → C 中需要调整: arr[i-1]
```

**代码生成器实现**:

```cpp
void CodeGenerator::visit(ArrayAccessNode& n) {
    output << n.array_name << "[";
    n.index->accept(*this);
    output << "]";
}
```

### 6.6 完整示例

**Pascal-S 输入**:
```pascal
program main;
var
  a: integer;
  b: integer;

function add(x: integer; y: integer): integer;
begin
  add := x + y;
end;

begin
  a := 5;
  b := add(a, 10);
  write(b);
end.
```

**生成的 C99 代码**:
```c
#include <stdio.h>

int add(int x, int y) {
    return x + y;
}

int main() {
    int a;
    int b;
    
    a = 5;
    b = add(a, 10);
    printf("%d", b);
    
    return 0;
}
```

---

## 7. 边缘情况与异常处理

### 7.1 词法错误

| 错误类型 | 示例 | 处理方式 |
|----------|------|----------|
| 非法字符 | `@`, `$`, `#` | 报告错误位置，跳过该字符 |
| 未终止字符串 | `'hello` | 报告错误，继续到行末 |
| 未终止注释 | `{ comment` | 报告错误，继续分析 |

**Flex 错误处理**:
```lex
/* 非法字符 */
. {
    ErrorHandler::instance().lexical_error(
        "Illegal character: " + std::string(yytext),
        yylineno,
        yycolumn
    );
    // 跳过非法字符，继续分析
}
```

### 7.2 语法错误 (恐慌模式恢复)

**恐慌模式 (Panic Mode) 策略**:

当 Bison 检测到语法错误时：

1. **进入恐慌模式**: 设置 `in_panic_mode = true`
2. **丢弃 Token**: 持续丢弃输入 Token，直到找到同步点
3. **同步点**: 语句分隔符 (`;`), `end`, `until` 等
4. **恢复分析**: 找到同步点后，退出恐慌模式，继续分析

**Bison 错误恢复**:
```yacc
// 在语句规则中添加错误恢复
statement:
    variable ASSIGN expression
    | error SEMICOLON
    {
        ErrorHandler::instance().panic_mode();
        // 跳过当前语句，继续分析下一条
    }
    ;

// 在声明中添加错误恢复
var_declaration_list:
    var_declaration_list IDENTIFIER COLON type_declaration SEMICOLON
    | error SEMICOLON
    {
        ErrorHandler::instance().synchronize();
    }
    ;
```

**同步 Token 集合**:
```cpp
// 同步 Token (用于恐慌模式恢复)
const std::set<TokenType> SYNC_TOKENS = {
    TokenType::DELIM_SEMICOLON,
    TokenType::KW_END,
    TokenType::KW_ELSE,
    TokenType::KW_UNTIL,
    TokenType::TOK_EOF
};
```

### 7.3 语义错误

| 错误类型 | 检测时机 | 示例 |
|----------|----------|------|
| 重复声明 | 符号表插入时 | `var a: integer; var a: real;` |
| 未定义标识符 | 符号表查找失败 | `x := y + 1;` (y 未定义) |
| 类型不匹配 | 类型检查阶段 | `a := 'x' + 1;` (char + int) |
| 作用域错误 | 符号表作用域检查 | 在过程外访问局部变量 |
| 参数不匹配 | 函数调用检查 | `func(1)` vs `func(x: int; y: int)` |

**语义分析器实现**:
```cpp
class SemanticAnalyzer : public ASTVisitor {
public:
    void visit(IdentifierNode& n) override {
        auto entry = symbol_table.lookup(n.name);
        if (!entry) {
            ErrorHandler::instance().semantic_error(
                "Undefined identifier: " + n.name,
                n.line, n.column
            );
        }
    }
    
    void visit(AssignmentNode& n) override {
        n.target->accept(*this);
        n.value->accept(*this);
        
        // 类型检查
        DataType target_type = get_expression_type(n.target);
        DataType value_type = get_expression_type(n.value);
        
        if (!TypeSystem::is_compatible(value_type, target_type)) {
            ErrorHandler::instance().semantic_error(
                "Type mismatch in assignment",
                n.line, n.column
            );
        }
    }
    
    void visit(FunctionDeclarationNode& n) override {
        // 检查重复声明
        if (symbol_table.exists_in_current_scope(n.func_name)) {
            ErrorHandler::instance().semantic_error(
                "Duplicate declaration: " + n.func_name,
                n.line, n.column
            );
            return;
        }
        
        // 添加到符号表
        symbol_table.add_function(n.func_name, n.return_type, n.parameters);
        
        // 进入函数作用域
        symbol_table.enter_scope();
        
        // 添加参数到符号表
        for (const auto& param : n.parameters) {
            symbol_table.insert(param.name, param.type, false, param.is_reference);
        }
        
        // 分析函数体
        n.body->accept(*this);
        
        // 退出作用域
        symbol_table.exit_scope();
    }
};
```

### 7.4 防雪崩机制

**目标**: 单个错误不应导致大量级联错误报告

**策略**:

1. **错误计数限制**: 最多报告 N 个错误后停止
2. **错误抑制**: 同一位置的错误只报告一次
3. **恐慌模式**: 严重错误后跳过当前语句/块
4. **错误分组**: 将相关错误分组报告

**实现**:
```cpp
class ErrorHandler {
private:
    static const int MAX_ERRORS = 20;
    std::set<std::pair<int, int>> reported_positions;  // 已报告位置
    
public:
    void semantic_error(const std::string& msg, int line, int col) {
        // 检查是否已报告过此位置的错误
        auto pos = std::make_pair(line, col);
        if (reported_positions.count(pos)) return;
        
        // 检查错误数量限制
        if (errors.size() >= MAX_ERRORS) {
            errors.push_back({
                ErrorType::SEMANTIC,
                "Too many errors. Compilation aborted.",
                line, col, false
            });
            return;
        }
        
        reported_positions.insert(pos);
        errors.push_back({ErrorType::SEMANTIC, msg, line, col, false});
    }
    
    bool should_continue() const {
        return errors.size() < MAX_ERRORS && !has_critical_errors();
    }
};
```

---

## 8. 项目目录结构

```
pascal-s-compiler/
├── docs/
│   └── architecture.md          # 本架构文档
├── src/
│   ├── main.cpp                 # 编译器入口
│   ├── compiler.hpp             # 编译器主类
│   ├── lexer.l                  # Flex 词法定义
│   ├── parser.y                 # Bison 语法定义
│   ├── ast/
│   │   ├── ast_node.hpp         # AST 基类
│   │   ├── expressions.hpp      # 表达式节点
│   │   ├── statements.hpp       # 语句节点
│   │   └── declarations.hpp     # 声明节点
│   ├── symbol_table.hpp         # 符号表实现
│   ├── semantic_analyzer.hpp    # 语义分析器
│   ├── code_generator.hpp       # 代码生成器
│   └── error_handler.hpp        # 错误处理器
├── tests/
│   ├── unit/                    # 单元测试
│   └── integration/             # 集成测试
├── open_set/                    # 官方测试样例 (80 个)
├── CMakeLists.txt               # CMake 构建配置
└── Makefile                     # Make 构建配置
```

---

## 9. 构建与运行

### 9.1 依赖要求

- **编译器**: g++ 9+ (支持 C++17)
- **工具**: Bison 3.0+, Flex 2.6+
- **构建系统**: CMake 3.10+ 或 Make

### 9.2 构建步骤

```bash
# 使用 CMake
mkdir build && cd build
cmake ..
make

# 或使用 Make
make all
```

### 9.3 运行示例

```bash
# 编译 Pascal-S 程序
./pascal-s-compiler open_set/00_main.pas -o output.c

# 编译并运行 (需要 gcc)
./pascal-s-compiler open_set/00_main.pas -o output.c
gcc -std=c99 output.c -o output
./output
```

---

## 10. 测试策略

### 10.1 测试覆盖

| 测试类别 | 文件数 | 覆盖内容 |
|----------|--------|----------|
| 基础语法 | 00-07 | 程序结构、变量/常量定义、函数声明 |
| 算术运算 | 08-17 | 加减乘除、取模 |
| 控制流 | 18-21 | if、while、for |
| 数组 | 03, 57 | 一维数组访问 |
| 函数/过程 | 06, 25, 57-58 | 定义、调用、递归、多参数 |
| 参数传递 | 57 (var) | 值传递、引用传递 |

### 10.2 测试自动化

```bash
# 运行所有测试
make test

# 单个测试
./run_test.sh open_set/00_main.pas
```

---

## 11. 后续扩展建议

1. **多维数组支持**: 扩展 `ArrayInfo` 结构，支持 `array [0..9, 0..9] of T`
2. **记录类型 (record)**: 添加结构体映射，需要 C struct 生成
3. **指针支持**: 谨慎实现，需内存安全检查
4. **优化阶段**: 添加中间表示 (IR)，进行常量折叠、死代码消除
5. **调试信息**: 生成 DWARF 调试信息，支持 gdb 调试

---

## 附录 A: Token 完整列表

| Token | 类型 | 示例 |
|-------|------|------|
| `PROGRAM` | 关键字 | `program` |
| `CONST` | 关键字 | `const` |
| `VAR` | 关键字 | `var` |
| `FUNCTION` | 关键字 | `function` |
| `PROCEDURE` | 关键字 | `procedure` |
| `BEGIN` | 关键字 | `begin` |
| `END` | 关键字 | `end` |
| `IF` | 关键字 | `if` |
| `THEN` | 关键字 | `then` |
| `ELSE` | 关键字 | `else` |
| `WHILE` | 关键字 | `while` |
| `DO` | 关键字 | `do` |
| `FOR` | 关键字 | `for` |
| `TO` | 关键字 | `to` |
| `DOWNTO` | 关键字 | `downto` |
| `INTEGER` | 类型 | `integer` |
| `REAL` | 类型 | `real` |
| `BOOLEAN` | 类型 | `boolean` |
| `CHAR` | 类型 | `char` |
| `ARRAY` | 类型 | `array` |
| `OF` | 关键字 | `of` |
| `NOT` | 运算符 | `not` |
| `AND` | 运算符 | `and` |
| `DIV` | 运算符 | `div` |
| `MOD` | 运算符 | `mod` |
| `ASSIGN` | 运算符 | `:=` |
| `RELOP` | 运算符 | `=`, `<>`, `<`, `<=`, `>`, `>=` |
| `ADDOP` | 运算符 | `+`, `-`, `or` |
| `MULOP` | 运算符 | `*`, `/`, `div`, `mod`, `and` |
| `IDENTIFIER` | 标识符 | `myVar`, `count` |
| `INTEGER_LITERAL` | 字面量 | `123`, `0` |
| `REAL_LITERAL` | 字面量 | `3.14`, `0.5` |
| `STRING_LITERAL` | 字面量 | `'hello'` |
| `LPAREN` | 分隔符 | `(` |
| `RPAREN` | 分隔符 | `)` |
| `LBRACKET` | 分隔符 | `[` |
| `RBRACKET` | 分隔符 | `]` |
| `SEMICOLON` | 分隔符 | `;` |
| `COMMA` | 分隔符 | `,` |
| `COLON` | 分隔符 | `:` |
| `DOT` | 分隔符 | `.` |
| `DOTDOT` | 分隔符 | `..` |

---

**文档状态**: ✅ 阶段 1 完成  
**下一步**: @engineer 准备接手实现

---

*最后更新：2026-03-22*  
*作者：月见八千代 (OpenClaw Agent)*
