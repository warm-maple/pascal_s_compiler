# Pascal-S 编译器架构设计文档

**版本**: 1.0  
**日期**: 2026-03-21  
**目标**: Pascal-S → C99 Source-to-Source 编译器  

---

## 1. 背景与目标

### 1.1 Pascal-S 语言特性概述

Pascal-S 是 Pascal 语言的教学子集，由瑞士苏黎世联邦理工学院 (ETH) 开发，用于编译器构造教学。它保留了 Pascal 的核心特性，同时简化了实现复杂度。

**核心特性**:
- 强类型系统 (整数、实数、布尔、字符、数组)
- 结构化控制流 (if/then/else, while, for, repeat/until, case)
- 过程与函数 (支持值参数和引用参数)
- 嵌套作用域 (块结构)
- 标准过程/函数 (read, write, readln, writeln, abs, sqr, sqrt 等)

### 1.2 编译器目标

**主要目标**:
1. 将完整的 Pascal-S 源代码转换为等价的 C99 代码
2. 实现单遍编译 (Single-Pass)，语法制导翻译
3. 生成可读、可维护的 C99 代码，便于调试和学习
4. 提供清晰的错误诊断和恢复机制

**技术目标**:
- 纯 C 标准库依赖 (stdio.h, stdlib.h, stdbool.h, string.h, math.h)
- 跨平台兼容 (Linux, macOS, Windows + MinGW)
- 模块化设计，便于扩展和测试

### 1.3 非目标 (明确排除)

以下 Pascal-S 特性**不在本编译器支持范围内**:

| 特性 | 排除原因 |
|------|----------|
| `record` (记录类型) | 需要复杂的内存布局处理，超出教学范围 |
| `pointer` (指针类型) | 涉及手动内存管理，与 C99 自动转换复杂 |
| `file I/O` (文件类型) | Pascal 的 file 类型语义与 C 差异较大 |
| `set` (集合类型) | 需要位运算实现，增加复杂度 |
| `goto` 语句 | 结构化编程原则，且 C99 标签转换复杂 |
| `variant records` | 依赖 record，已排除 |

---

## 2. 系统架构

### 2.1 整体编译流程图

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Pascal-S 源代码                               │
│                        (input.ps)                                   │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                          Lexer (词法分析器)                          │
│  输入：字符流 → 输出：Token 流                                       │
│  功能：标识符、关键字、运算符、字面量识别                             │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        Parser (语法分析器)                           │
│  输入：Token 流 → 输出：AST (抽象语法树)                              │
│  功能：递归下降分析，语法制导翻译                                     │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    Semantic Analyzer (语义分析器)                    │
│  输入：AST → 输出：标注后的 AST                                       │
│  功能：类型检查、作用域分析、符号表管理                               │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     Code Generator (代码生成器)                      │
│  输入：标注后的 AST → 输出：C99 源代码                                │
│  功能：AST 遍历，C 代码 emit，变量/过程映射                           │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         C99 源代码                                   │
│                        (output.c)                                   │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      Symbol Table (符号表)                           │
│  全程维护：两级栈式结构 (全局 scope + 局部 scope 栈)                   │
└─────────────────────────────────────────────────────────────────────┘
                                   ▲
                                   │
┌─────────────────────────────────────────────────────────────────────┐
│                       Error Handler (错误处理器)                     │
│  全程监控：词法/语法/语义错误收集与报告                               │
└─────────────────────────────────────────────────────────────────────┘
```

### 2.2 核心模块划分

| 模块 | 文件 | 职责 |
|------|------|------|
| **Lexer** | `lexer.c/h` | 字符流 → Token 流，关键字识别，字面量解析 |
| **Parser** | `parser.c/h` | Token 流 → AST，递归下降分析 |
| **Semantic Analyzer** | `semantic.c/h` | 类型检查、作用域验证、符号表操作 |
| **Code Generator** | `codegen.c/h` | AST → C99 代码，字符串拼接与格式化 |
| **Symbol Table** | `symbol_table.c/h` | 符号管理、作用域栈、查找与插入 |
| **Error Handler** | `error.c/h` | 错误收集、报告、恐慌模式恢复 |
| **Main Driver** | `main.c` | 命令行解析、模块协调、文件 I/O |

### 2.3 模块间数据流

```
Lexer ──Token──> Parser ──AST──> Semantic Analyzer ──Annotated AST──> Code Generator ──C Code──> Output
   │                │                    │                                    │
   │                │                    │                                    │
   └────────────────┴────────────────────┴────────────────────────────────────┘
                                    │
                                    ▼
                              Symbol Table (共享)
                                    │
                                    ▼
                              Error Handler (共享)
```

**数据流特点**:
- **单遍编译**: Lexer、Parser、Semantic Analyzer、Code Generator 顺序执行，无回溯
- **共享状态**: Symbol Table 和 Error Handler 为全局共享，所有模块可访问
- **增量输出**: Code Generator 直接写入输出文件，无需中间表示存储

---

## 3. 核心数据结构定义 (C99 风格)

### 3.1 Token 结构体

```c
// token.h
#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>
#include <stdint.h>

// Token 类型枚举
typedef enum {
    // 关键字
    TOK_PROGRAM, TOK_CONST, TOK_VAR, TOK_PROCEDURE, TOK_FUNCTION,
    TOK_BEGIN, TOK_END, TOK_IF, TOK_THEN, TOK_ELSE, TOK_WHILE,
    TOK_DO, TOK_FOR, TOK_TO, TOK_DOWNTO, TOK_REPEAT, TOK_UNTIL,
    TOK_CASE, TOK_OF, TOK_OTHERWISE, TOK_RETURN,
    
    // 数据类型
    TOK_INTEGER, TOK_REAL, TOK_BOOLEAN, TOK_CHAR, TOK_ARRAY,
    
    // 运算符
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_ASSIGN,
    TOK_EQ, TOK_NE, TOK_LT, TOK_LE, TOK_GT, TOK_GE,
    TOK_AND, TOK_OR, TOK_NOT,
    
    // 分隔符
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACKET, TOK_RBRACKET,
    TOK_COMMA, TOK_SEMICOLON, TOK_COLON, TOK_DOT,
    
    // 字面量与标识符
    TOK_IDENT, TOK_INT_LITERAL, TOK_REAL_LITERAL,
    TOK_CHAR_LITERAL, TOK_STRING_LITERAL,
    
    // 特殊
    TOK_EOF, TOK_ERROR
} TokenType;

// Token 结构体
typedef struct {
    TokenType type;           // Token 类型
    char *lexeme;             // 原始词素 (动态分配)
    int line;                 // 行号 (1-based)
    int column;               // 列号 (1-based)
    
    // 字面量值 (union 节省内存)
    union {
        int64_t int_val;      // 整数 literal
        double real_val;      // 实数 literal
        char char_val;        // 字符 literal
    } value;
    
    // 类型信息 (语义分析阶段填充)
    void *type_info;          // 指向 Type 结构的指针
} Token;

// Token 创建与销毁
Token *token_create(TokenType type, const char *lexeme, int line, int column);
void token_free(Token *token);

#endif // TOKEN_H
```

### 3.2 SymbolEntry 结构体 (两级栈式符号表)

```c
// symbol_table.h
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>
#include <stdint.h>

// 符号种类
typedef enum {
    SYM_VARIABLE,       // 变量
    SYM_CONSTANT,       // 常量
    SYM_PROCEDURE,      // 过程
    SYM_FUNCTION,       // 函数
    SYM_PARAMETER,      // 参数 (值参/引用参)
    SYM_ARRAY           // 数组
} SymbolKind;

// 类型信息
typedef enum {
    TYPE_INTEGER,
    TYPE_REAL,
    TYPE_BOOLEAN,
    TYPE_CHAR,
    TYPE_VOID,          // 用于过程
    TYPE_ARRAY,         // 数组类型
    TYPE_UNKNOWN        // 未定义/错误
} BaseType;

typedef struct Type {
    BaseType base;              // 基础类型
    struct Type *element_type;  // 数组元素类型 (仅 TYPE_ARRAY 有效)
    int array_size;             // 数组大小 (仅 TYPE_ARRAY 有效)
    bool is_reference;          // 是否为引用类型 (VAR 参数)
} Type;

// 符号表条目
typedef struct SymbolEntry {
    char *name;                 // 符号名称 (动态分配)
    SymbolKind kind;            // 符号种类
    Type *type;                 // 类型信息
    int scope_level;            // 作用域层级 (0=全局，1+=嵌套)
    int offset;                 // 在作用域内的偏移 (用于代码生成)
    
    // 过程/函数特有信息
    struct {
        int param_count;        // 参数个数
        struct SymbolEntry **params;  // 参数列表
        Type *return_type;      // 返回值类型 (仅函数)
        bool is_forward;        // 是否前向声明
    } proc_info;
    
    // 常量值
    union {
        int64_t int_const;
        double real_const;
        char char_const;
        bool bool_const;
    } const_value;
    
    // 链式哈希冲突处理
    struct SymbolEntry *next;
} SymbolEntry;

// 符号表 (栈式作用域)
typedef struct {
    SymbolEntry **scopes;       // 作用域栈 (数组)
    int scope_count;            // 当前作用域层级
    int scope_capacity;         // 作用域栈容量
    int total_symbols;          // 符号总数
} SymbolTable;

// 符号表操作
SymbolTable *symbol_table_create(void);
void symbol_table_destroy(SymbolTable *table);

void symbol_table_push_scope(SymbolTable *table);
void symbol_table_pop_scope(SymbolTable *table);

SymbolEntry *symbol_table_insert(SymbolTable *table, const char *name,
                                  SymbolKind kind, Type *type);
SymbolEntry *symbol_table_lookup(SymbolTable *table, const char *name);
SymbolEntry *symbol_table_lookup_current_scope(SymbolTable *table, const char *name);

bool symbol_table_exists(SymbolTable *table, const char *name);

#endif // SYMBOL_TABLE_H
```

### 3.3 TreeNode 结构体 (AST)

```c
// ast.h
#ifndef AST_H
#define AST_H

#include "token.h"
#include "symbol_table.h"

// AST 节点类型
typedef enum {
    // 程序结构
    NODE_PROGRAM,
    NODE_BLOCK,
    NODE_DECL_SECTION,
    NODE_VAR_DECL,
    NODE_CONST_DECL,
    NODE_PROC_DECL,
    NODE_FUNC_DECL,
    
    // 语句
    NODE_ASSIGN_STMT,
    NODE_IF_STMT,
    NODE_WHILE_STMT,
    NODE_FOR_STMT,
    NODE_REPEAT_STMT,
    NODE_CASE_STMT,
    NODE_RETURN_STMT,
    NODE_PROC_CALL,
    NODE_COMPOUND_STMT,
    NODE_EMPTY_STMT,
    
    // 表达式
    NODE_BINARY_EXPR,
    NODE_UNARY_EXPR,
    NODE_IDENT_EXPR,
    NODE_LITERAL_EXPR,
    NODE_ARRAY_ACCESS,
    NODE_FUNC_CALL,
    
    // 类型
    NODE_TYPE_SPEC,
    NODE_ARRAY_TYPE
} NodeType;

// AST 节点
typedef struct TreeNode {
    NodeType type;              // 节点类型
    Token *token;               // 关联的 Token (用于错误报告)
    
    // 子节点 (动态数组)
    struct TreeNode **children;
    int child_count;
    int child_capacity;
    
    // 节点特有数据 (union)
    union {
        // 标识符/字面量
        char *ident_name;
        int64_t int_literal;
        double real_literal;
        char char_literal;
        bool bool_literal;
        
        // 二元/一元表达式
        struct {
            struct TreeNode *left;
            struct TreeNode *right;
            TokenType op;
        } expr;
        
        struct {
            struct TreeNode *operand;
            TokenType op;
        } unary;
        
        // 数组访问
        struct {
            struct TreeNode *array;
            struct TreeNode *index;
        } array_access;
        
        // 函数/过程调用
        struct {
            char *name;
            struct TreeNode **args;
            int arg_count;
        } call;
        
        // 变量声明
        struct {
            char *var_name;
            Type *type;
        } var_decl;
        
        // 常量声明
        struct {
            char *const_name;
            Type *type;
            union {
                int64_t int_val;
                double real_val;
                char char_val;
                bool bool_val;
            } value;
        } const_decl;
        
        // 过程/函数声明
        struct {
            char *name;
            Type *return_type;  // NULL 表示过程
            struct TreeNode *params;
            struct TreeNode *body;
            bool is_forward;
        } proc_decl;
    } data;
    
    // 语义分析结果
    Type *resolved_type;        // 解析后的类型
    SymbolEntry *symbol;        // 关联的符号表条目
    
    // 代码生成辅助
    char *c_temp_var;           // 生成的 C 临时变量名
    int c_label;                // 生成的 C 标签号
} TreeNode;

// AST 节点操作
TreeNode *ast_node_create(NodeType type, Token *token);
void ast_node_add_child(TreeNode *parent, TreeNode *child);
void ast_node_free(TreeNode *node);

// AST 遍历 (用于语义分析和代码生成)
typedef void (*AstVisitor)(TreeNode *node, void *context);
void ast_traverse(TreeNode *node, AstVisitor visitor, void *context);

#endif // AST_H
```

### 3.4 类型系统定义

```c
// type_system.h
#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include "symbol_table.h"

// 类型兼容性检查
typedef enum {
    TYPE_COMPATIBLE,
    TYPE_ASSIGNMENT_COMPATIBLE,
    TYPE_INCOMPATIBLE
} TypeCompatibility;

// 类型操作函数
Type *type_create(BaseType base);
Type *type_create_array(Type *element_type, int size);
void type_free(Type *type);

// 类型比较
bool type_equals(Type *a, Type *b);
TypeCompatibility type_check_compatibility(Type *expected, Type *actual);

// 类型提升 (用于算术运算)
Type *type_promote(Type *a, Type *b);

// 类型名称 (用于错误报告)
const char *type_name(Type *type);

// 标准类型 (单例)
extern Type *type_integer;
extern Type *type_real;
extern Type *type_boolean;
extern Type *type_char;
extern Type *type_void;

#endif // TYPE_SYSTEM_H
```

---

## 4. 接口定义

### 4.1 Lexer 模块 API

```c
// lexer.h
#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <stdio.h>

typedef struct {
    FILE *input;                // 输入文件
    char *filename;             // 文件名 (用于错误报告)
    int line;                   // 当前行号
    int column;                 // 当前列号
    char current_char;          // 当前字符
    bool eof;                   // 是否到达文件末尾
} Lexer;

// Lexer 生命周期
Lexer *lexer_create(const char *filename);
void lexer_destroy(Lexer *lexer);

// Token 获取
Token *lexer_next_token(Lexer *lexer);
Token *lexer_peek_token(Lexer *lexer);  // 预读一个 Token

// 错误处理
const char *lexer_get_error(Lexer *lexer);
int lexer_get_error_count(Lexer *lexer);

#endif // LEXER_H
```

### 4.2 Parser 模块 API

```c
// parser.h
#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"
#include "lexer.h"

typedef struct {
    Lexer *lexer;               // 词法分析器
    Token *current_token;       // 当前 Token
    Token *peek_token;          // 预读 Token
    TreeNode *ast;              // 生成的 AST 根节点
    int error_count;            // 错误计数
} Parser;

// Parser 生命周期
Parser *parser_create(Lexer *lexer);
void parser_destroy(Parser *parser);

// 解析入口
TreeNode *parser_parse(Parser *parser);

// 错误处理
const char *parser_get_error(Parser *parser);
int parser_get_error_count(Parser *parser);

#endif // PARSER_H
```

### 4.3 Semantic Analyzer 模块 API

```c
// semantic.h
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol_table.h"

typedef struct {
    SymbolTable *symbol_table;  // 符号表
    int error_count;            // 错误计数
    char last_error[256];       // 最后一条错误信息
} SemanticAnalyzer;

// Analyzer 生命周期
SemanticAnalyzer *semantic_analyzer_create(SymbolTable *symbol_table);
void semantic_analyzer_destroy(SemanticAnalyzer *analyzer);

// 语义分析入口
bool semantic_analyze(SemanticAnalyzer *analyzer, TreeNode *ast);

// 错误处理
const char *semantic_get_error(SemanticAnalyzer *analyzer);
int semantic_get_error_count(SemanticAnalyzer *analyzer);

#endif // SEMANTIC_H
```

### 4.4 Code Generator 模块 API

```c
// codegen.h
#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symbol_table.h"
#include <stdio.h>

typedef struct {
    FILE *output;               // 输出文件
    SymbolTable *symbol_table;  // 符号表
    int indent_level;           // 当前缩进层级
    int temp_var_counter;       // 临时变量计数器
    int label_counter;          // 标签计数器
    int error_count;            // 错误计数
} CodeGenerator;

// Generator 生命周期
CodeGenerator *codegen_create(FILE *output, SymbolTable *symbol_table);
void codegen_destroy(CodeGenerator *generator);

// 代码生成入口
bool codegen_generate(CodeGenerator *generator, TreeNode *ast, const char *program_name);

// 辅助函数
void codegen_emit(CodeGenerator *generator, const char *format, ...);
void codegen_emit_indent(CodeGenerator *generator);
void codegen_emit_line(CodeGenerator *generator, const char *format, ...);

#endif // CODEGEN_H
```

### 4.5 Error Handler 模块 API

```c
// error.h
#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

typedef enum {
    ERROR_LEXICAL,
    ERROR_SYNTAX,
    ERROR_SEMANTIC,
    ERROR_CODEGEN
} ErrorType;

typedef enum {
    ERROR_FATAL,      // 编译终止
    ERROR_RECOVERABLE // 继续编译
} ErrorSeverity;

typedef struct {
    ErrorType type;
    ErrorSeverity severity;
    char *message;
    char *filename;
    int line;
    int column;
} CompilerError;

typedef struct {
    CompilerError **errors;
    int error_count;
    int error_capacity;
    int max_errors;         // 最大错误数 (防雪崩)
    bool has_fatal;         // 是否有致命错误
} ErrorHandler;

// Error Handler 生命周期
ErrorHandler *error_handler_create(void);
void error_handler_destroy(ErrorHandler *handler);

// 错误报告
void error_report(ErrorHandler *handler, ErrorType type, ErrorSeverity severity,
                  const char *message, const char *filename, int line, int column);

// 快捷宏
#define LEXER_ERROR(msg, line, col) \
    error_report(handler, ERROR_LEXICAL, ERROR_RECOVERABLE, msg, filename, line, col)

#define SYNTAX_ERROR(msg, line, col) \
    error_report(handler, ERROR_SYNTAX, ERROR_RECOVERABLE, msg, filename, line, col)

#define SEMANTIC_ERROR(msg, line, col) \
    error_report(handler, ERROR_SEMANTIC, ERROR_RECOVERABLE, msg, filename, line, col)

// 错误输出
void error_print_all(ErrorHandler *handler);
bool error_has_fatal(ErrorHandler *handler);

#endif // ERROR_H
```

---

## 5. Pascal-S 语法子集定义

### 5.1 支持的数据类型

```
<type> ::= integer
         | real
         | boolean
         | char
         | array [ <constant> .. <constant> ] of <type>
```

**示例**:
```pascal
var
    x: integer;
    y: real;
    flag: boolean;
    ch: char;
    arr: array [1..10] of integer;
    matrix: array [1..3, 1..3] of real;  { 多维数组 (语法糖：数组的数组) }
```

### 5.2 支持的语句类型

```
<statement> ::= <assignment>
              | <procedure_call>
              | <compound_statement>
              | <if_statement>
              | <while_statement>
              | <for_statement>
              | <repeat_statement>
              | <case_statement>
              | <return_statement>
              | <empty_statement>

<assignment> ::= <variable> := <expression>

<compound_statement> ::= begin <statement> { ; <statement> } end

<if_statement> ::= if <expression> then <statement> [ else <statement> ]

<while_statement> ::= while <expression> do <statement>

<for_statement> ::= for <variable> := <expression> (to|downto) <expression> do <statement>

<repeat_statement> ::= repeat <statement> { ; <statement> } until <expression>

<case_statement> ::= case <expression> of
                         <constant> : <statement> { ; <constant> : <statement> }
                         [ otherwise : <statement> ]
                     end

<return_statement> ::= return [ <expression> ]

<empty_statement> ::=  { 空语句，仅分号 }
```

### 5.3 支持的表达式

```
<expression> ::= <simple_expression> [ <relation> <simple_expression> ]

<simple_expression> ::= [ <sign> ] <term> { <adding_operator> <term> }

<term> ::= <factor> { <multiplying_operator> <factor> }

<factor> ::= <variable>
           | <unsigned_constant>
           | <function_call>
           | ( <expression> )
           | not <factor>

<relation> ::= = | <> | < | <= | > | >=

<adding_operator> ::= + | - | or

<multiplying_operator> ::= * | / | div | mod | and

<sign> ::= + | -
```

**运算符优先级** (从高到低):
1. `not` (一元)
2. `*`, `/`, `div`, `mod`, `and`
3. `+`, `-`, `or`
4. `=`, `<>`, `<`, `<=`, `>`, `>=`

### 5.4 声明语法

```
<program> ::= program <identifier> ( <input_files> ) ;
              <block> .

<block> ::= [ <const_section> ]
            [ <var_section> ]
            [ <proc_and_func_section> ]
            <statement>

<const_section> ::= const <constant_definition> { ; <constant_definition> } ;

<constant_definition> ::= <identifier> = <constant>

<var_section> ::= var <variable_declaration> { ; <variable_declaration> } ;

<variable_declaration> ::= <identifier> { , <identifier> } : <type>

<proc_and_func_section> ::= <procedure_declaration> | <function_declaration>
                            { ; <procedure_declaration> | <function_declaration> }

<procedure_declaration> ::= procedure <identifier> [ <formal_parameters> ] ; <block>

<function_declaration> ::= function <identifier> [ <formal_parameters> ] : <type> ; <block>

<formal_parameters> ::= ( <parameter_group> { ; <parameter_group> } )

<parameter_group> ::= [ var ] <identifier> { , <identifier> } : <type>
```

---

## 6. 代码生成策略

### 6.1 Pascal-S 构造到 C99 的映射规则

| Pascal-S 构造 | C99 映射 | 说明 |
|---------------|----------|------|
| `program name;` | `int main(void) {` | 程序入口转换为 main 函数 |
| `var x: integer;` | `int x;` | 变量声明直接映射 |
| `var x: real;` | `double x;` | real → double |
| `var x: boolean;` | `bool x;` | 需要 `#include <stdbool.h>` |
| `var x: char;` | `char x;` | 直接映射 |
| `x := expr;` | `x = expr;` | 赋值运算符 |
| `begin ... end` | `{ ... }` | 复合语句块 |
| `if cond then S` | `if (cond) { S }` | 条件语句 |
| `if cond then S1 else S2` | `if (cond) { S1 } else { S2 }` | 条件语句 |
| `while cond do S` | `while (cond) { S }` | 循环语句 |
| `for i := a to b do S` | `for (int i = a; i <= b; i++) { S }` | for 循环 |
| `for i := a downto b do S` | `for (int i = a; i >= b; i--) { S }` | for 循环 (递减) |
| `repeat S until cond` | `do { S } while (!(cond));` | repeat-until 循环 |
| `x and y` | `(x) && (y)` | 布尔与 |
| `x or y` | `(x) || (y)` | 布尔或 |
| `not x` | `!(x)` | 布尔非 |
| `x = y` | `(x) == (y)` | 相等比较 |
| `x <> y` | `(x) != (y)` | 不等比较 |
| `x div y` | `(x) / (y)` | 整数除法 |
| `x mod y` | `(x) % (y)` | 取模运算 |
| `write(x)` | `printf("%d", x);` | 输出 (根据类型选择格式) |
| `writeln(x)` | `printf("%d\n", x);` | 输出带换行 |
| `read(x)` | `scanf("%d", &x);` | 输入 |
| `readln(x)` | `scanf("%d\n", &x);` | 输入带换行 |

### 6.2 变量声明转换

**Pascal-S**:
```pascal
program Example;
var
    x, y: integer;
    z: real;
    flag: boolean;
    arr: array [1..10] of integer;
const
    MAX = 100;
    PI = 3.14159;
```

**生成的 C99**:
```c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX 100
#define PI 3.14159

int main(void) {
    int x, y;
    double z;
    bool flag;
    int arr[10];  // Pascal: 1..10, C: 0..9，需要索引调整
    
    // ... 程序体
    return 0;
}
```

**数组索引调整**:
- Pascal-S 数组索引可以是任意范围 (如 `1..10`, `0..9`, `-5..5`)
- C 数组索引从 0 开始
- 代码生成时需要插入偏移计算：`arr[i]` → `arr[(i) - 1]` (对于 `1..10` 的数组)

### 6.3 控制流转换

#### 6.3.1 If 语句

**Pascal-S**:
```pascal
if x > 0 then
    y := x * 2
else
    y := -x;
```

**C99**:
```c
if (x > 0) {
    y = x * 2;
} else {
    y = -x;
}
```

#### 6.3.2 While 循环

**Pascal-S**:
```pascal
while i <= n do
begin
    sum := sum + i;
    i := i + 1
end;
```

**C99**:
```c
while (i <= n) {
    sum = sum + i;
    i = i + 1;
}
```

#### 6.3.3 For 循环 (to)

**Pascal-S**:
```pascal
for i := 1 to 10 do
    write(i);
```

**C99**:
```c
for (int i = 1; i <= 10; i++) {
    printf("%d", i);
}
```

#### 6.3.4 For 循环 (downto)

**Pascal-S**:
```pascal
for i := 10 downto 1 do
    write(i);
```

**C99**:
```c
for (int i = 10; i >= 1; i--) {
    printf("%d", i);
}
```

#### 6.3.5 Repeat-Until 循环

**Pascal-S**:
```pascal
repeat
    read(x);
    sum := sum + x
until x = 0;
```

**C99**:
```c
do {
    scanf("%d", &x);
    sum = sum + x;
} while (!(x == 0));
```

#### 6.3.6 Case 语句

**Pascal-S**:
```pascal
case option of
    1: writeln('One');
    2: writeln('Two');
    3: writeln('Three');
    otherwise: writeln('Other')
end;
```

**C99**:
```c
switch (option) {
    case 1:
        printf("One\n");
        break;
    case 2:
        printf("Two\n");
        break;
    case 3:
        printf("Three\n");
        break;
    default:
        printf("Other\n");
        break;
}
```

### 6.4 过程/函数调用转换

#### 6.4.1 参数传递机制

Pascal-S 支持两种参数传递方式:
- **值参数**: 默认，传递副本
- **引用参数**: 使用 `var` 关键字，传递引用 (类似 C 指针)

**Pascal-S**:
```pascal
procedure Swap(var a, b: integer);
var
    temp: integer;
begin
    temp := a;
    a := b;
    b := temp
end;

procedure PrintValue(x: integer);  { 值参数 }
begin
    writeln(x)
end;
```

**C99 转换策略**:
- 值参数：直接传递
- 引用参数：转换为指针，调用时取地址

```c
void Swap(int *a, int *b) {
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

void PrintValue(int x) {
    printf("%d\n", x);
}

// 调用
Swap(&x, &y);      // var 参数需要取地址
PrintValue(z);     // 值参数直接传递
```

#### 6.4.2 is_ref 参数处理

在代码生成器中，需要为每个参数维护 `is_ref` 标记:

```c
typedef struct {
    char *name;
    Type *type;
    bool is_ref;    // true 表示 var 参数
} ParameterInfo;

// 代码生成时
if (param.is_ref) {
    codegen_emit("&%s", arg_name);  // 生成 &arg
} else {
    codegen_emit("%s", arg_name);   // 直接传递
}
```

#### 6.4.3 函数返回值

**Pascal-S**:
```pascal
function Max(a, b: integer): integer;
begin
    if a > b then
        Max := a
    else
        Max := b
end;
```

**C99**:
```c
int Max(int a, int b) {
    int _return_value;  // 临时返回值变量
    if (a > b) {
        _return_value = a;
    } else {
        _return_value = b;
    }
    return _return_value;
}
```

**实现策略**:
1. 为每个函数创建一个 `_return_value` 临时变量
2. 将 Pascal-S 中的 `FuncName := expr` 转换为 `_return_value = expr;`
3. 在函数末尾添加 `return _return_value;`

#### 6.4.4 标准过程/函数映射

| Pascal-S | C99 | 头文件 |
|----------|-----|--------|
| `abs(x)` | `abs(x)` / `fabs(x)` | stdlib.h / math.h |
| `sqr(x)` | `(x) * (x)` | - |
| `sqrt(x)` | `sqrt(x)` | math.h |
| `sin(x)` | `sin(x)` | math.h |
| `cos(x)` | `cos(x)` | math.h |
| `exp(x)` | `exp(x)` | math.h |
| `ln(x)` | `log(x)` | math.h |
| `odd(x)` | `((x) % 2) != 0` | - |
| `chr(x)` | `(char)(x)` | - |
| `ord(x)` | `(int)(x)` | - |
| `trunc(x)` | `(int)(x)` | - |
| `round(x)` | `(int)((x) + 0.5)` | - |

---

## 7. 边缘情况与异常处理

### 7.1 词法错误

**错误类型**:
- 非法字符 (如 `@`, `$`, `%` 等 Pascal-S 不支持的字符)
- 未终止的字符串/字符字面量
- 数字格式错误 (如 `123.45.67`)
- 注释未闭合

**处理策略**:
```c
// lexer.c
static void handle_illegal_char(Lexer *lexer, char ch) {
    error_report(handler, ERROR_LEXICAL, ERROR_RECOVERABLE,
                 "非法字符 '%c'", lexer->filename, lexer->line, lexer->column);
    lexer_advance(lexer);  // 跳过非法字符，继续处理
}

static void handle_unterminated_string(Lexer *lexer) {
    error_report(handler, ERROR_LEXICAL, ERROR_FATAL,
                 "未终止的字符串字面量", lexer->filename, lexer->line, lexer->column);
    lexer->eof = true;  // 终止词法分析
}
```

**错误恢复**:
- 非法字符：跳过该字符，继续处理下一个
- 未终止字面量：终止编译 (FATAL)

### 7.2 语法错误 (恐慌模式恢复)

**恐慌模式 (Panic Mode)**:
当检测到语法错误时，丢弃输入 Token 直到找到**同步 Token**，然后恢复解析。

**同步 Token 集合**:
```c
static const TokenType SYNC_TOKENS[] = {
    TOK_SEMICOLON,    // 语句结束
    TOK_END,          // 块结束
    TOK_ELSE,         // if 语句
    TOK_UNTIL,        // repeat 语句
    TOK_EOF,          // 文件结束
    // 声明同步
    TOK_CONST, TOK_VAR, TOK_PROCEDURE, TOK_FUNCTION,
    // 语句同步
    TOK_BEGIN, TOK_IF, TOK_WHILE, TOK_FOR, TOK_REPEAT, TOK_CASE,
    TOK_ASSIGN, TOK_IDENT
};

static bool is_sync_token(TokenType type) {
    for (int i = 0; i < sizeof(SYNC_TOKENS)/sizeof(SYNC_TOKENS[0]); i++) {
        if (SYNC_TOKENS[i] == type) return true;
    }
    return false;
}

// 恐慌模式恢复
static void panic_mode_recover(Parser *parser) {
    while (parser->current_token->type != TOK_EOF) {
        if (is_sync_token(parser->current_token->type)) {
            // 找到同步点，恢复解析
            return;
        }
        parser_advance(parser);  // 丢弃 Token
    }
}
```

**示例**:
```pascal
begin
    x := 10;
    y := 20  { 缺少分号 }
    z := x + y;
end.
```

解析器在 `y := 20` 后期望分号，发现 `z` (标识符) 时报错，进入恐慌模式，丢弃 Token 直到找到 `;` 或 `end`，然后恢复。

### 7.3 语义错误

#### 7.3.1 类型检查错误

**错误类型**:
- 类型不匹配 (如 `x := 'a'`，x 是 integer)
- 运算符类型错误 (如 `3 + 'a'`)
- 函数返回类型不匹配
- 数组索引类型错误 (必须是 integer)

**处理策略**:
```c
// semantic.c
static void check_assignment_type(SemanticAnalyzer *analyzer,
                                   Type *var_type, Type *expr_type,
                                   int line, int column) {
    TypeCompatibility compat = type_check_compatibility(var_type, expr_type);
    
    if (compat == TYPE_INCOMPATIBLE) {
        error_report(analyzer->errors, ERROR_SEMANTIC, ERROR_RECOVERABLE,
                     "类型不匹配：无法将 '%s' 赋值给 '%s' 类型变量",
                     analyzer->filename, line, column,
                     type_name(expr_type), type_name(var_type));
    }
}
```

#### 7.3.2 重复声明错误

**错误类型**:
- 同一作用域内重复声明变量/常量/过程
- 参数名重复
- 变量名与过程名冲突

**处理策略**:
```c
// symbol_table.c
SymbolEntry *symbol_table_insert(SymbolTable *table, const char *name,
                                  SymbolKind kind, Type *type) {
    // 检查当前作用域是否已存在
    if (symbol_table_lookup_current_scope(table, name) != NULL) {
        error_report(handler, ERROR_SEMANTIC, ERROR_RECOVERABLE,
                     "重复声明：'%s' 已在当前作用域中声明",
                     filename, line, column, name);
        return NULL;  // 插入失败
    }
    
    // 创建并插入新条目
    SymbolEntry *entry = symbol_entry_create(name, kind, type);
    // ... 插入逻辑
    return entry;
}
```

#### 7.3.3 未声明标识符

**错误类型**:
- 使用未声明的变量
- 调用未声明的过程/函数
- 数组名拼写错误

**处理策略**:
```c
// semantic.c
static void check_identifier(SemanticAnalyzer *analyzer, const char *name,
                              int line, int column) {
    SymbolEntry *entry = symbol_table_lookup(analyzer->symbol_table, name);
    
    if (entry == NULL) {
        error_report(analyzer->errors, ERROR_SEMANTIC, ERROR_RECOVERABLE,
                     "未声明的标识符：'%s'", analyzer->filename, line, column, name);
        // 创建占位符号，避免后续错误雪崩
        entry = symbol_table_insert(analyzer->symbol_table, name,
                                    SYM_VARIABLE, type_integer);  // 默认类型
    }
    
    return entry;
}
```

### 7.4 防雪崩机制

**问题**: 一个早期错误可能导致大量后续错误报告，淹没真正的错误原因。

**解决方案**:

#### 7.4.1 最大错误数限制

```c
// error.h
#define MAX_ERRORS 25  // 最大错误报告数

typedef struct {
    // ...
    int max_errors;
    bool has_fatal;
} ErrorHandler;

// error.c
void error_report(ErrorHandler *handler, ErrorType type, ErrorSeverity severity,
                  const char *message, const char *filename, int line, int column) {
    if (handler->has_fatal) {
        return;  // 已有致命错误，不再报告
    }
    
    if (handler->error_count >= handler->max_errors) {
        if (handler->error_count == handler->max_errors) {
            fprintf(stderr, "\n[错误] 错误数量过多，已停止报告 (最多 %d 条)\n", MAX_ERRORS);
            handler->error_count++;  // 只报告一次
        }
        return;
    }
    
    // 正常报告错误
    // ...
}
```

#### 7.4.2 错误抑制 (Error Suppression)

当检测到某些错误模式时，抑制相关的后续错误:

```c
// 示例：未终止的括号
if (unterminated_paren_detected) {
    suppress_errors_until(TOK_RPAREN);  // 抑制直到右括号
    panic_mode_recover(parser);
}
```

#### 7.4.3 占位符号 (Placeholder Symbols)

当遇到未声明标识符时，创建一个占位符号，避免后续重复报告:

```c
// 见 7.3.3 节 check_identifier 函数
// 创建默认类型的占位符号，后续使用不再报错
```

#### 7.4.4 错误分类汇总

编译结束时，输出错误汇总:

```
编译完成，发现 15 个错误：
  - 词法错误：2
  - 语法错误：5
  - 语义错误：8

前 25 个错误已报告，后续错误已抑制。
```

---

## 附录 A: 项目文件结构

```
pascal-s-compiler/
├── src/
│   ├── main.c              # 程序入口
│   ├── lexer.c/h           # 词法分析器
│   ├── parser.c/h          # 语法分析器
│   ├── ast.c/h             # AST 定义与操作
│   ├── semantic.c/h        # 语义分析器
│   ├── symbol_table.c/h    # 符号表管理
│   ├── codegen.c/h         # 代码生成器
│   ├── error.c/h           # 错误处理
│   ├── type_system.c/h     # 类型系统
│   └── utils.c/h           # 工具函数
├── include/                # 公共头文件
├── tests/                  # 测试用例
│   ├── lexical/
│   ├── syntax/
│   ├── semantic/
│   └── codegen/
├── examples/               # Pascal-S 示例程序
├── docs/
│   └── architecture.md     # 本架构文档
├── Makefile                # 构建配置
└── README.md               # 项目说明
```

---

## 附录 B: 编译与运行

### 构建

```bash
make
# 或
gcc -std=c99 -Wall -Wextra -O2 -o pascalsc src/*.c
```

### 使用

```bash
./pascalsc input.ps -o output.c
# 或
./pascalsc input.ps  # 默认输出 output.c
```

### 测试

```bash
make test
# 运行所有测试用例
```

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0 | 2026-03-21 | 初始架构设计完成 |

---

**阶段 1 完成** ✅

@engineer 准备接手

---

*文档结束*
