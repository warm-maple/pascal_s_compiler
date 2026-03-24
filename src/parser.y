%{
#include "ast.h"
#include "symbol_table.h"
#include "error.h"
#include <cstring>
#include <vector>
#include <algorithm>

extern int yylex();
void yyerror(const char* m);
extern int yylineno;

extern pascal_s::ProgramNode* root_ast;
extern pascal_s::SymbolTable g_symbol_table;

// 全局临时存储
static std::vector<pascal_s::ParameterInfo> func_params;
static std::vector<pascal_s::StatementNode*> stmt_list;
static std::vector<std::vector<pascal_s::StatementNode*>> stmt_list_stack;  // stmt_list 栈
static std::vector<std::vector<pascal_s::ExpressionNode*>> arg_list_stack;  // arg_list 栈
static pascal_s::StatementNode* stmt_result;
static pascal_s::StatementNode* then_branch_temp = nullptr;  // 用于 if-else 保存 then 分支
static std::string last_func_name;
static std::vector<pascal_s::FunctionDeclarationNode*> pending_func_decls;  // 待处理的函数声明
static std::vector<pascal_s::VariableDeclarationNode*> pending_var_decls;   // 待处理的变量声明（全局）
static std::vector<std::vector<pascal_s::VariableDeclarationNode*>> pending_local_var_decls;  // 局部变量栈
static pascal_s::ArrayInfo last_array_info;  // 最后解析的数组类型信息

static pascal_s::BinaryOp relop_to_binop(const char* op) {
    if (strcmp(op, "=") == 0) return pascal_s::BinaryOp::OP_EQ;
    if (strcmp(op, "<>") == 0) return pascal_s::BinaryOp::OP_NE;
    if (strcmp(op, "<") == 0) return pascal_s::BinaryOp::OP_LT;
    if (strcmp(op, "<=") == 0) return pascal_s::BinaryOp::OP_LE;
    if (strcmp(op, ">") == 0) return pascal_s::BinaryOp::OP_GT;
    if (strcmp(op, ">=") == 0) return pascal_s::BinaryOp::OP_GE;
    return pascal_s::BinaryOp::OP_EQ;
}
%}

%union {
    int ival;
    double dval;
    char* sval;
    char cval;
    pascal_s::DataType tval;
    pascal_s::ExpressionNode* expr;
    pascal_s::StatementNode* stmt;
    std::vector<char*>* name_list_t;
    std::vector<pascal_s::ExpressionNode*>* index_list_t;
}

%token <sval> IDENTIFIER
%token <ival> INTEGER_LITERAL
%token <dval> REAL_LITERAL
%token <sval> STRING_LITERAL
%token <cval> CHAR_LITERAL

%token PROGRAM CONST VAR FUNCTION PROCEDURE
%token BEGIN_KW END IF THEN ELSE WHILE DO FOR TO DOWNTO
%token INTEGER REAL BOOLEAN CHAR ARRAY OF
%token NOT AND OR
%token DIV MOD
%token WRITE WRITELN

%token ASSIGN
%token RELOP ADDOP MULOP

%token LPAREN RPAREN LBRACKET RBRACKET SEMICOLON COMMA COLON DOT DOTDOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%type <tval> type_decl
%type <sval> ADDOP MULOP RELOP
%type <expr> expr simple_expr term factor
%type <name_list_t> name_list
%type <index_list_t> index_lst

%define parse.error verbose


%%

prog: PROGRAM IDENTIFIER SEMICOLON block DOT {
    root_ast = new pascal_s::ProgramNode($2);
    // 添加所有暂存的函数声明到 AST
    for (auto* func_decl : pending_func_decls) {
        root_ast->add_declaration(std::unique_ptr<pascal_s::FunctionDeclarationNode>(func_decl));
    }
    pending_func_decls.clear();
    // 添加所有暂存的变量声明到 AST
    for (auto* var_decl : pending_var_decls) {
        root_ast->add_declaration(std::unique_ptr<pascal_s::VariableDeclarationNode>(var_decl));
    }
    pending_var_decls.clear();
    root_ast->main_body.reset(static_cast<pascal_s::CompoundStatementNode*>(stmt_result));
    free($2);
};

block: any_decls compound_stmt;

any_decls: any_decls const_decl
         | any_decls var_decl
         | any_decls subprog
         | /* empty */
;

const_decl: CONST const_list;
var_decl: VAR var_list;

const_list: const_list const_item | const_item;
const_item: IDENTIFIER COLON type_decl RELOP expr SEMICOLON {
    g_symbol_table.insert($1, $3, true);
    auto var_decl = new pascal_s::VariableDeclarationNode($1, $3);
    var_decl->is_const = true;
    var_decl->init_value.reset($5);
    pending_var_decls.push_back(var_decl);
    free($1);
}
| IDENTIFIER RELOP expr SEMICOLON {
    // 根据表达式类型推断常量类型
    pascal_s::DataType dtype = pascal_s::DataType::TY_INTEGER;
    if (dynamic_cast<pascal_s::CharLiteralNode*>($3)) {
        dtype = pascal_s::DataType::TY_CHAR;
    } else if (dynamic_cast<pascal_s::RealLiteralNode*>($3)) {
        dtype = pascal_s::DataType::TY_REAL;
    } else if (dynamic_cast<pascal_s::StringLiteralNode*>($3)) {
        dtype = pascal_s::DataType::TY_CHAR;  // 字符串常量使用 char 类型标记
    }
    g_symbol_table.insert($1, dtype, true);
    auto var_decl = new pascal_s::VariableDeclarationNode($1, dtype);
    var_decl->is_const = true;
    var_decl->init_value.reset($3);
    pending_var_decls.push_back(var_decl);
    free($1);
};

var_list: var_list var_def | var_def;
var_def: name_list COLON type_decl SEMICOLON {
    for (char* name : *$1) {
        g_symbol_table.insert(name, $3, false, false, last_array_info);
        auto var_decl = new pascal_s::VariableDeclarationNode(name, $3);
        var_decl->is_array = ($3 == pascal_s::DataType::TY_ARRAY);
        var_decl->array_info = last_array_info;
        // 如果在函数内部，添加到局部变量列表
        if (!pending_local_var_decls.empty()) {
            pending_local_var_decls.back().push_back(var_decl);
        } else {
            pending_var_decls.push_back(var_decl);
        }
        free(name);
    }
    delete $1;
    // 重置数组信息
    last_array_info = pascal_s::ArrayInfo{};
};
name_list: IDENTIFIER {
    $$ = new std::vector<char*>();
    $$->push_back($1);
}
| name_list COMMA IDENTIFIER {
    $1->push_back($3);
    $$ = $1;
};

type_decl: INTEGER { $$ = pascal_s::DataType::TY_INTEGER; } | REAL { $$ = pascal_s::DataType::TY_REAL; }
         | BOOLEAN { $$ = pascal_s::DataType::TY_BOOLEAN; } | CHAR { $$ = pascal_s::DataType::TY_CHAR; }
         | array_type_decl { $$ = pascal_s::DataType::TY_ARRAY; };

array_type_decl: ARRAY LBRACKET range_list RBRACKET OF type_decl {
    // last_array_info 已经在 range_list 中构建完成
    last_array_info.element_type = $<tval>6;  // type_decl 是第 6 个符号
    // 向后兼容：设置第一个维度的 bounds
    if (!last_array_info.dimensions.empty()) {
        last_array_info.lower_bound = last_array_info.dimensions[0].lower_bound;
        last_array_info.upper_bound = last_array_info.dimensions[0].upper_bound;
    }
};

range_list: INTEGER_LITERAL DOTDOT INTEGER_LITERAL {
    last_array_info = pascal_s::ArrayInfo{};  // 重置
    pascal_s::ArrayDimension dim;
    dim.lower_bound = $1;
    dim.upper_bound = $3;
    last_array_info.dimensions.push_back(dim);
}
| range_list COMMA INTEGER_LITERAL DOTDOT INTEGER_LITERAL {
    // last_array_info 已经包含之前的维度
    pascal_s::ArrayDimension dim;
    dim.lower_bound = $3;
    dim.upper_bound = $5;
    last_array_info.dimensions.push_back(dim);
};
subprog: func_hdr block SEMICOLON {
    // 函数体已存储在 stmt_result 中，函数声明在 func_hdr 中已创建并添加到符号表
    // 创建函数声明节点并暂存
    auto func_decl = new pascal_s::FunctionDeclarationNode(last_func_name, false);
    auto sym_entry = g_symbol_table.lookup(last_func_name);
    if (sym_entry) {
        func_decl->return_type = sym_entry->return_type;
        func_decl->parameters = sym_entry->params;
    }
    // 添加局部变量
    if (!pending_local_var_decls.empty()) {
        for (auto* var : pending_local_var_decls.back()) {
            func_decl->add_local_var(std::unique_ptr<pascal_s::VariableDeclarationNode>(var));
        }
        pending_local_var_decls.pop_back();
    }
    func_decl->body.reset(static_cast<pascal_s::CompoundStatementNode*>(stmt_result));
    pending_func_decls.push_back(func_decl);
    g_symbol_table.exit_scope();
    stmt_result = nullptr;  // 已转移到 AST，清空
};

func_hdr: FUNCTION IDENTIFIER params COLON type_decl SEMICOLON {
    last_func_name = $2;
    g_symbol_table.add_function($2, $5, func_params, false);
    g_symbol_table.enter_scope();
    func_params.clear();  // 清除参数列表，防止累积
    pending_local_var_decls.push_back({});  // 新建局部变量列表
    free($2);
}
| FUNCTION IDENTIFIER SEMICOLON {
    last_func_name = $2;
    g_symbol_table.add_function($2, pascal_s::DataType::TY_INTEGER, {}, false);
    g_symbol_table.enter_scope();
    pending_local_var_decls.push_back({});  // 新建局部变量列表
    free($2);
}
| PROCEDURE IDENTIFIER params SEMICOLON {
    last_func_name = $2;
    g_symbol_table.add_function($2, pascal_s::DataType::TY_VOID, func_params, true);
    g_symbol_table.enter_scope();
    func_params.clear();  // 清除参数列表，防止累积
    pending_local_var_decls.push_back({});  // 新建局部变量列表
    free($2);
}
| PROCEDURE IDENTIFIER SEMICOLON {
    last_func_name = $2;
    g_symbol_table.add_function($2, pascal_s::DataType::TY_VOID, {}, true);
    g_symbol_table.enter_scope();
    pending_local_var_decls.push_back({});  // 新建局部变量列表
    free($2);
};

params: LPAREN param_lst RPAREN { /* func_params already populated */ }
      | LPAREN RPAREN { func_params.clear(); }
      | { func_params.clear(); };
param_lst: param_lst SEMICOLON param_grp | param_grp;
param_grp: VAR name_list COLON type_decl {
    for (char* name : *$2) {
        pascal_s::ParameterInfo p; p.name = name; p.type = $4; p.is_reference = true;
        func_params.push_back(p); g_symbol_table.add_parameter(name, $4, true); free(name);
    }
    delete $2;
}
| name_list COLON type_decl {
    for (char* name : *$1) {
        pascal_s::ParameterInfo p; p.name = name; p.type = $3; p.is_reference = false;
        func_params.push_back(p); g_symbol_table.add_parameter(name, $3, false); free(name);
    }
    delete $1;
};

compound_stmt: BEGIN_KW {
    stmt_list_stack.push_back(stmt_list);
    stmt_list.clear();
} stmt_seq END {
    auto cs = new pascal_s::CompoundStatementNode();
    // 去重：避免重复添加相同的语句指针
    std::vector<pascal_s::StatementNode*> seen;
    for (auto& s : stmt_list) {
        if (s && std::find(seen.begin(), seen.end(), s) == seen.end()) {
            cs->add_statement(std::unique_ptr<pascal_s::StatementNode>(s));
            seen.push_back(s);
        }
    }
    stmt_list.clear();
    if (!stmt_list_stack.empty()) {
        stmt_list = stmt_list_stack.back();
        stmt_list_stack.pop_back();
    }
    stmt_result = cs;
};

stmt_seq: stmt_seq SEMICOLON stmt | stmt;

stmt: IDENTIFIER ASSIGN expr {
    stmt_result = new pascal_s::AssignmentNode(
        new pascal_s::IdentifierNode($1), $3);
    stmt_list.push_back(stmt_result);
    $<stmt>$ = stmt_result;
    free($1);
}
| IDENTIFIER LBRACKET index_lst RBRACKET ASSIGN expr {
    auto* arr = new pascal_s::ArrayAccessNode($1);
    for (auto& idx : *$3) {
        arr->add_index(std::unique_ptr<pascal_s::ExpressionNode>(idx));
    }
    delete $3;
    stmt_result = new pascal_s::AssignmentNode(arr, $6);
    stmt_list.push_back(stmt_result);
    $<stmt>$ = stmt_result;
    free($1);
}
| compound_stmt {
    stmt_list.push_back(stmt_result);
    $<stmt>$ = stmt_result;
}
| if_stmt { stmt_list.push_back(stmt_result); $<stmt>$ = stmt_result; }
| while_stmt { stmt_list.push_back(stmt_result); $<stmt>$ = stmt_result; }
| for_stmt { stmt_list.push_back(stmt_result); $<stmt>$ = stmt_result; }
| proc_call { stmt_list.push_back(stmt_result); $<stmt>$ = stmt_result; }
| write_stmt { stmt_list.push_back(stmt_result); $<stmt>$ = stmt_result; }
| /* empty */ { stmt_result = nullptr; $<stmt>$ = nullptr; }
;

if_stmt: IF expr THEN stmt ELSE stmt {
    // 直接从 bison 栈读取 then-branch ($4) 和 else-branch ($6)
    // 无需 mid-rule action，无需全局 then_branch_temp
    auto* ts = $<stmt>4;
    auto* es = $<stmt>6;
    if (!stmt_list.empty() && stmt_list.back() == es) stmt_list.pop_back();
    if (!stmt_list.empty() && stmt_list.back() == ts) stmt_list.pop_back();
    auto* ifs = new pascal_s::IfStatementNode($2, ts);
    ifs->set_else_branch(std::unique_ptr<pascal_s::StatementNode>(es));
    stmt_result = ifs;
}
| IF expr THEN stmt %prec LOWER_THAN_ELSE {
    auto* ts = $<stmt>4;
    if (!stmt_list.empty() && stmt_list.back() == ts) stmt_list.pop_back();
    stmt_result = new pascal_s::IfStatementNode($2, ts);
};

while_stmt: WHILE expr DO stmt {
    auto* body = $<stmt>4;
    if (!stmt_list.empty() && stmt_list.back() == body) stmt_list.pop_back();
    stmt_result = new pascal_s::WhileStatementNode($2, body);
};

for_stmt: FOR IDENTIFIER ASSIGN expr TO expr DO stmt {
    auto* body = $<stmt>8;
    if (!stmt_list.empty() && stmt_list.back() == body) stmt_list.pop_back();
    stmt_result = new pascal_s::ForStatementNode($2, $4, $6, body, false);
    free($2);
}
| FOR IDENTIFIER ASSIGN expr DOWNTO expr DO stmt {
    auto* body = $<stmt>8;
    if (!stmt_list.empty() && stmt_list.back() == body) stmt_list.pop_back();
    stmt_result = new pascal_s::ForStatementNode($2, $4, $6, body, true);
    free($2);
};

proc_call: IDENTIFIER LPAREN { arg_list_stack.push_back({}); } arg_lst RPAREN {
    auto c = new pascal_s::ProcedureCallNode($1);
    for (auto& a : arg_list_stack.back()) c->add_argument(std::unique_ptr<pascal_s::ExpressionNode>(a));
    arg_list_stack.pop_back();
    stmt_result = c;
    free($1);
}
| IDENTIFIER LPAREN RPAREN {
    auto c = new pascal_s::ProcedureCallNode($1);
    stmt_result = c;
    free($1);
}
| IDENTIFIER {
    auto c = new pascal_s::ProcedureCallNode($1);
    stmt_result = c;
    free($1);
};

write_stmt: WRITE LPAREN { arg_list_stack.push_back({}); } expr_lst RPAREN {
    auto* ws = new pascal_s::WriteStatementNode(nullptr);
    for (auto* e : arg_list_stack.back()) {
        ws->values.push_back(std::unique_ptr<pascal_s::ExpressionNode>(e));
    }
    arg_list_stack.pop_back();
    stmt_result = ws;
}
| WRITELN LPAREN { arg_list_stack.push_back({}); } expr_lst RPAREN {
    auto* ws = new pascal_s::WriteStatementNode(nullptr);
    for (auto* e : arg_list_stack.back()) {
        ws->values.push_back(std::unique_ptr<pascal_s::ExpressionNode>(e));
    }
    arg_list_stack.pop_back();
    stmt_result = ws;
}
| WRITE LPAREN RPAREN {
    stmt_result = new pascal_s::WriteStatementNode(nullptr);
}
| WRITELN LPAREN RPAREN {
    stmt_result = new pascal_s::WriteStatementNode(nullptr);
};

expr_lst: expr_lst COMMA expr { arg_list_stack.back().push_back($3); }
        | expr { arg_list_stack.back().push_back($1); };

expr: simple_expr { $$ = $1; }
| simple_expr RELOP simple_expr {
    $$ = new pascal_s::BinaryExpressionNode(relop_to_binop($2), 
        std::unique_ptr<pascal_s::ExpressionNode>($1),
        std::unique_ptr<pascal_s::ExpressionNode>($3));
    free($2);
};

simple_expr: term { $$ = $1; }
| simple_expr ADDOP term {
    if (strcmp($2, "+") == 0) {
        $$ = new pascal_s::BinaryExpressionNode(pascal_s::BinaryOp::OP_ADD,
            std::unique_ptr<pascal_s::ExpressionNode>($1),
            std::unique_ptr<pascal_s::ExpressionNode>($3));
    } else {
        $$ = new pascal_s::BinaryExpressionNode(pascal_s::BinaryOp::OP_SUB,
            std::unique_ptr<pascal_s::ExpressionNode>($1),
            std::unique_ptr<pascal_s::ExpressionNode>($3));
    }
    free($2);
}
| simple_expr OR term {
    $$ = new pascal_s::BinaryExpressionNode(pascal_s::BinaryOp::OP_OR,
        std::unique_ptr<pascal_s::ExpressionNode>($1),
        std::unique_ptr<pascal_s::ExpressionNode>($3));
};

term: factor { $$ = $1; }
| term MULOP factor {
    if (strcmp($2, "*") == 0) {
        $$ = new pascal_s::BinaryExpressionNode(pascal_s::BinaryOp::OP_MUL,
            std::unique_ptr<pascal_s::ExpressionNode>($1),
            std::unique_ptr<pascal_s::ExpressionNode>($3));
    } else {
        $$ = new pascal_s::BinaryExpressionNode(pascal_s::BinaryOp::OP_DIV_REAL,
            std::unique_ptr<pascal_s::ExpressionNode>($1),
            std::unique_ptr<pascal_s::ExpressionNode>($3));
    }
    free($2);
}
| term DIV factor {
    $$ = new pascal_s::BinaryExpressionNode(pascal_s::BinaryOp::OP_DIV,
        std::unique_ptr<pascal_s::ExpressionNode>($1),
        std::unique_ptr<pascal_s::ExpressionNode>($3));
}
| term MOD factor {
    $$ = new pascal_s::BinaryExpressionNode(pascal_s::BinaryOp::OP_MOD,
        std::unique_ptr<pascal_s::ExpressionNode>($1),
        std::unique_ptr<pascal_s::ExpressionNode>($3));
}
| term AND factor {
    $$ = new pascal_s::BinaryExpressionNode(pascal_s::BinaryOp::OP_AND,
        std::unique_ptr<pascal_s::ExpressionNode>($1),
        std::unique_ptr<pascal_s::ExpressionNode>($3));
};

factor: INTEGER_LITERAL { $$ = new pascal_s::IntegerLiteralNode($1); }
| REAL_LITERAL { $$ = new pascal_s::RealLiteralNode($1); }
| CHAR_LITERAL { $$ = new pascal_s::CharLiteralNode($1); }
| STRING_LITERAL { $$ = new pascal_s::StringLiteralNode($1); free($1); }
| IDENTIFIER { $$ = new pascal_s::IdentifierNode($1); free($1); }
| IDENTIFIER LPAREN { arg_list_stack.push_back({}); } arg_lst RPAREN {
    auto c = new pascal_s::FunctionCallNode($1);
    for (auto& a : arg_list_stack.back()) c->add_argument(std::unique_ptr<pascal_s::ExpressionNode>(a));
    arg_list_stack.pop_back(); $$ = c; free($1);
}
| IDENTIFIER LPAREN RPAREN {
    auto c = new pascal_s::FunctionCallNode($1);
    $$ = c; free($1);
}
| IDENTIFIER LBRACKET index_lst RBRACKET {
    auto* node = new pascal_s::ArrayAccessNode($1);
    for (auto& idx : *$3) {
        node->add_index(std::unique_ptr<pascal_s::ExpressionNode>(idx));
    }
    delete $3;
    $$ = node;
    free($1);
}
| LPAREN expr RPAREN { $$ = $2; }
| NOT factor {
    $$ = new pascal_s::UnaryExpressionNode(pascal_s::UnaryOp::UOP_NOT,
        std::unique_ptr<pascal_s::ExpressionNode>($2));
}
| ADDOP factor {
    if (strcmp($1, "-") == 0) {
        $$ = new pascal_s::UnaryExpressionNode(pascal_s::UnaryOp::UOP_NEGATE,
            std::unique_ptr<pascal_s::ExpressionNode>($2));
    } else {
        $$ = $2;
    }
    free($1);
};

arg_lst: arg_lst COMMA expr { arg_list_stack.back().push_back($3); } | expr { arg_list_stack.back().push_back($1); };

index_lst: index_lst COMMA expr { $1->push_back($3); $$ = $1; }
         | expr { $$ = new std::vector<pascal_s::ExpressionNode*>(); $$->push_back($1); };

%%

void yyerror(const char* m) {
    pascal_s::ErrorHandler::instance().syntax_error(m, yylineno, 0);
}
