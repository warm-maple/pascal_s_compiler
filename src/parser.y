%{
#include "ast.h"
#include "error.h"
#include "parser_state.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

extern int yylex();
void yyerror(const char* m);
extern int yylineno;
extern int yycolumn_start;
extern int yyleng;

extern pascal_s::ProgramNode* root_ast;

static pascal_s::ParserState& ctx() { return pascal_s::parser_state(); }


template <typename T>
static T* set_node_position(T* node, int line = yylineno, int column = yycolumn_start) {
    if (node) {
        node->line = line;
        node->column = column;
    }
    return node;
}

template <typename T>
static T* adopt_node_position(T* node, const pascal_s::ASTNode* source) {
    if (node && source) {
        node->line = source->line;
        node->column = source->column;
    } else {
        set_node_position(node);
    }
    return node;
}

static void reset_type_side_data() {
    ctx().reset_type_side_data();
}

static void assign_record_struct_names(pascal_s::RecordInfo& record_info) {
    if (record_info.struct_name.empty()) {
        record_info.struct_name = "pas_record_" + std::to_string(++ctx().next_record_struct_id);
    }
    for (auto& field : record_info.fields) {
        if (field.record_info) {
            assign_record_struct_names(*field.record_info);
        }
    }
}

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
%token <ival> BOOLEAN_LITERAL
%token <sval> STRING_LITERAL
%token <cval> CHAR_LITERAL

%token PROGRAM CONST VAR FUNCTION PROCEDURE
%token BEGIN_KW END IF THEN ELSE WHILE DO FOR TO DOWNTO
%token INTEGER REAL BOOLEAN CHAR ARRAY OF RECORD
%token NOT AND OR
%token DIV MOD
%token WRITE WRITELN

%token ASSIGN
%token RELOP ADDOP MULOP

%token LPAREN RPAREN LBRACKET RBRACKET SEMICOLON COMMA COLON DOT DOTDOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%type <tval> type_decl array_type_decl record_type_decl
%type <sval> ADDOP MULOP RELOP
%type <expr> expr simple_expr term factor variable_ref
%type <name_list_t> name_list
%type <index_list_t> index_lst


%%

prog: PROGRAM IDENTIFIER program_params_opt SEMICOLON block DOT {
    root_ast = set_node_position(new pascal_s::ProgramNode($2));
    for (auto* func_decl : ctx().pending_func_decls) {
        root_ast->add_declaration(std::unique_ptr<pascal_s::FunctionDeclarationNode>(func_decl));
    }
    ctx().pending_func_decls.clear();
    for (auto* var_decl : ctx().pending_var_decls) {
        root_ast->add_declaration(std::unique_ptr<pascal_s::VariableDeclarationNode>(var_decl));
    }
    ctx().pending_var_decls.clear();
    root_ast->main_body.reset(static_cast<pascal_s::CompoundStatementNode*>(ctx().stmt_result));
    free($2);
};

program_params_opt:
      LPAREN name_list RPAREN {
          for (char* name : *$2) free(name);
          delete $2;
      }
    | /* empty */
;

block: any_decls compound_stmt;

any_decls: any_decls const_decl
         | any_decls var_decl
         | any_decls subprog
         | /* empty */
;

const_decl: CONST const_list;
var_decl: VAR var_list;

const_list: const_list const_item | const_item;
const_item:
      IDENTIFIER COLON type_decl RELOP expr SEMICOLON {
          auto* var_decl = set_node_position(new pascal_s::VariableDeclarationNode($1, $3));
          var_decl->is_const = true;
          var_decl->init_value.reset($5);
          ctx().pending_var_decls.push_back(var_decl);
          reset_type_side_data();
          free($1);
      }
    | IDENTIFIER RELOP expr SEMICOLON {
          pascal_s::DataType dtype = pascal_s::DataType::TY_INTEGER;
          if (dynamic_cast<pascal_s::BooleanLiteralNode*>($3)) {
              dtype = pascal_s::DataType::TY_BOOLEAN;
          } else if (dynamic_cast<pascal_s::CharLiteralNode*>($3)) {
              dtype = pascal_s::DataType::TY_CHAR;
          } else if (dynamic_cast<pascal_s::RealLiteralNode*>($3)) {
              dtype = pascal_s::DataType::TY_REAL;
          } else if (dynamic_cast<pascal_s::StringLiteralNode*>($3)) {
              dtype = pascal_s::DataType::TY_CHAR;
          }
          auto* var_decl = set_node_position(new pascal_s::VariableDeclarationNode($1, dtype));
          var_decl->is_const = true;
          var_decl->init_value.reset($3);
          ctx().pending_var_decls.push_back(var_decl);
          free($1);
      }
;

var_list: var_list var_def | var_def;
var_def: name_list COLON type_decl SEMICOLON {
    pascal_s::ArrayInfo array_info = ctx().last_array_info;
    pascal_s::RecordInfo record_info = ctx().last_record_info;
    for (char* name : *$1) {
        auto* var_decl = set_node_position(new pascal_s::VariableDeclarationNode(name, $3));
        var_decl->is_array = ($3 == pascal_s::DataType::TY_ARRAY);
        var_decl->array_info = array_info;
        if ($3 == pascal_s::DataType::TY_RECORD || array_info.element_type == pascal_s::DataType::TY_RECORD) {
            var_decl->record_info = record_info;
        }
        if (!ctx().pending_local_var_decls.empty()) {
            ctx().pending_local_var_decls.back().push_back(var_decl);
        } else {
            ctx().pending_var_decls.push_back(var_decl);
        }
        free(name);
    }
    delete $1;
    reset_type_side_data();
};
name_list: IDENTIFIER {
    $$ = new std::vector<char*>();
    $$->push_back($1);
}
| name_list COMMA IDENTIFIER {
    $1->push_back($3);
    $$ = $1;
};

type_decl:
      INTEGER { reset_type_side_data(); $$ = pascal_s::DataType::TY_INTEGER; }
    | REAL { reset_type_side_data(); $$ = pascal_s::DataType::TY_REAL; }
    | BOOLEAN { reset_type_side_data(); $$ = pascal_s::DataType::TY_BOOLEAN; }
    | CHAR { reset_type_side_data(); $$ = pascal_s::DataType::TY_CHAR; }
    | array_type_decl { $$ = $1; }
    | record_type_decl { $$ = $1; }
;

array_type_decl: ARRAY LBRACKET range_list RBRACKET OF {
    ctx().array_info_stack.push_back(ctx().last_array_info);
} type_decl {
    ctx().last_array_info = ctx().array_info_stack.back();
    ctx().array_info_stack.pop_back();
    ctx().last_array_info.element_type = $7;
    if (!ctx().last_array_info.dimensions.empty()) {
        ctx().last_array_info.lower_bound = ctx().last_array_info.dimensions[0].lower_bound;
        ctx().last_array_info.upper_bound = ctx().last_array_info.dimensions[0].upper_bound;
    }
    $$ = pascal_s::DataType::TY_ARRAY;
};

range_list: INTEGER_LITERAL DOTDOT INTEGER_LITERAL {
    ctx().last_array_info = pascal_s::ArrayInfo{};
    pascal_s::ArrayDimension dim;
    dim.lower_bound = $1;
    dim.upper_bound = $3;
    ctx().last_array_info.dimensions.push_back(dim);
}
| range_list COMMA INTEGER_LITERAL DOTDOT INTEGER_LITERAL {
    pascal_s::ArrayDimension dim;
    dim.lower_bound = $3;
    dim.upper_bound = $5;
    ctx().last_array_info.dimensions.push_back(dim);
};

record_type_decl:
      RECORD {
          ctx().record_info_stack.emplace_back();
      } record_field_list END {
          ctx().last_record_info = std::move(ctx().record_info_stack.back());
          ctx().record_info_stack.pop_back();
          assign_record_struct_names(ctx().last_record_info);
          $$ = pascal_s::DataType::TY_RECORD;
      }
;

record_field_list:
      record_field_list record_field
    | record_field
;

record_field:
      name_list COLON type_decl SEMICOLON {
          pascal_s::ArrayInfo array_info = ctx().last_array_info;
          pascal_s::RecordInfo record_info = ctx().last_record_info;
          for (char* name : *$1) {
              pascal_s::RecordField field;
              field.name = name;
              field.type = $3;
              field.is_array = ($3 == pascal_s::DataType::TY_ARRAY);
              field.array_info = array_info;
              if ($3 == pascal_s::DataType::TY_RECORD || array_info.element_type == pascal_s::DataType::TY_RECORD) {
                  field.record_info = std::make_shared<pascal_s::RecordInfo>(record_info);
              }
              ctx().record_info_stack.back().fields.push_back(std::move(field));
              free(name);
          }
          delete $1;
          reset_type_side_data();
      }
;
subprog: func_hdr block SEMICOLON {
    auto* func_decl = set_node_position(new pascal_s::FunctionDeclarationNode(ctx().last_func_name, ctx().last_func_is_proc));
    func_decl->return_type = ctx().last_func_return_type;
    func_decl->parameters = ctx().func_params;
    if (!ctx().pending_local_var_decls.empty()) {
        for (auto* var : ctx().pending_local_var_decls.back()) {
            func_decl->add_local_var(std::unique_ptr<pascal_s::VariableDeclarationNode>(var));
        }
        ctx().pending_local_var_decls.pop_back();
    }
    func_decl->body.reset(static_cast<pascal_s::CompoundStatementNode*>(ctx().stmt_result));
    ctx().pending_func_decls.push_back(func_decl);
    ctx().stmt_result = nullptr;
};

func_hdr: FUNCTION IDENTIFIER params COLON type_decl SEMICOLON {
    ctx().last_func_name = $2;
    ctx().last_func_return_type = $5;
    ctx().last_func_is_proc = false;
    ctx().pending_local_var_decls.push_back({});
    reset_type_side_data();
    free($2);
}
| FUNCTION IDENTIFIER COLON type_decl SEMICOLON {
    ctx().last_func_name = $2;
    ctx().last_func_return_type = $4;
    ctx().last_func_is_proc = false;
    ctx().func_params.clear();
    ctx().pending_local_var_decls.push_back({});
    reset_type_side_data();
    free($2);
}
| FUNCTION IDENTIFIER SEMICOLON {
    ctx().last_func_name = $2;
    ctx().last_func_return_type = pascal_s::DataType::TY_INTEGER;
    ctx().last_func_is_proc = false;
    ctx().func_params.clear();
    ctx().pending_local_var_decls.push_back({});
    free($2);
}
| PROCEDURE IDENTIFIER params SEMICOLON {
    ctx().last_func_name = $2;
    ctx().last_func_return_type = pascal_s::DataType::TY_VOID;
    ctx().last_func_is_proc = true;
    ctx().pending_local_var_decls.push_back({});
    free($2);
}
| PROCEDURE IDENTIFIER SEMICOLON {
    ctx().last_func_name = $2;
    ctx().last_func_return_type = pascal_s::DataType::TY_VOID;
    ctx().last_func_is_proc = true;
    ctx().func_params.clear();
    ctx().pending_local_var_decls.push_back({});
    free($2);
};

params: LPAREN { ctx().func_params.clear(); } param_lst RPAREN { }
      | LPAREN RPAREN { ctx().func_params.clear(); };
param_lst: param_lst SEMICOLON param_grp | param_grp;
param_grp: VAR name_list COLON type_decl {
    pascal_s::ArrayInfo array_info = ctx().last_array_info;
    pascal_s::RecordInfo record_info = ctx().last_record_info;
    for (char* name : *$2) {
        pascal_s::ParameterInfo p;
        p.name = name;
        p.type = $4;
        p.is_reference = true;
        p.array_info = array_info;
        if ($4 == pascal_s::DataType::TY_RECORD || array_info.element_type == pascal_s::DataType::TY_RECORD) {
            p.record_info = record_info;
        }
        ctx().func_params.push_back(p);
        free(name);
    }
    delete $2;
    reset_type_side_data();
}
| name_list COLON type_decl {
    pascal_s::ArrayInfo array_info = ctx().last_array_info;
    pascal_s::RecordInfo record_info = ctx().last_record_info;
    for (char* name : *$1) {
        pascal_s::ParameterInfo p;
        p.name = name;
        p.type = $3;
        p.is_reference = false;
        p.array_info = array_info;
        if ($3 == pascal_s::DataType::TY_RECORD || array_info.element_type == pascal_s::DataType::TY_RECORD) {
            p.record_info = record_info;
        }
        ctx().func_params.push_back(p);
        free(name);
    }
    delete $1;
    reset_type_side_data();
};

compound_stmt: BEGIN_KW {
    ctx().stmt_list_stack.push_back(ctx().stmt_list);
    ctx().stmt_list.clear();
} stmt_seq END {
    auto* cs = set_node_position(new pascal_s::CompoundStatementNode());
    std::vector<pascal_s::StatementNode*> seen;
    for (auto* s : ctx().stmt_list) {
        if (s && std::find(seen.begin(), seen.end(), s) == seen.end()) {
            cs->add_statement(std::unique_ptr<pascal_s::StatementNode>(s));
            seen.push_back(s);
        }
    }
    ctx().stmt_list.clear();
    if (!ctx().stmt_list_stack.empty()) {
        ctx().stmt_list = ctx().stmt_list_stack.back();
        ctx().stmt_list_stack.pop_back();
    }
    ctx().stmt_result = cs;
};

stmt_seq: stmt_seq SEMICOLON stmt | stmt;

stmt: variable_ref ASSIGN expr {
    ctx().stmt_result = adopt_node_position(new pascal_s::AssignmentNode($1, $3), $1);
    ctx().stmt_list.push_back(ctx().stmt_result);
    $<stmt>$ = ctx().stmt_result;
}
| compound_stmt {
    ctx().stmt_list.push_back(ctx().stmt_result);
    $<stmt>$ = ctx().stmt_result;
}
| if_stmt { ctx().stmt_list.push_back(ctx().stmt_result); $<stmt>$ = ctx().stmt_result; }
| while_stmt { ctx().stmt_list.push_back(ctx().stmt_result); $<stmt>$ = ctx().stmt_result; }
| for_stmt { ctx().stmt_list.push_back(ctx().stmt_result); $<stmt>$ = ctx().stmt_result; }
| proc_call { ctx().stmt_list.push_back(ctx().stmt_result); $<stmt>$ = ctx().stmt_result; }
| write_stmt { ctx().stmt_list.push_back(ctx().stmt_result); $<stmt>$ = ctx().stmt_result; }
| /* empty */ { ctx().stmt_result = nullptr; $<stmt>$ = nullptr; }
;

if_stmt: IF expr THEN stmt ELSE stmt {
    auto* ts = $<stmt>4;
    auto* es = $<stmt>6;
    if (!ctx().stmt_list.empty() && ctx().stmt_list.back() == es) ctx().stmt_list.pop_back();
    if (!ctx().stmt_list.empty() && ctx().stmt_list.back() == ts) ctx().stmt_list.pop_back();
    auto* ifs = adopt_node_position(new pascal_s::IfStatementNode($2, ts), $2);
    ifs->set_else_branch(std::unique_ptr<pascal_s::StatementNode>(es));
    ctx().stmt_result = ifs;
}
| IF expr THEN stmt %prec LOWER_THAN_ELSE {
    auto* ts = $<stmt>4;
    if (!ctx().stmt_list.empty() && ctx().stmt_list.back() == ts) ctx().stmt_list.pop_back();
    ctx().stmt_result = adopt_node_position(new pascal_s::IfStatementNode($2, ts), $2);
};

while_stmt: WHILE expr DO stmt {
    auto* body = $<stmt>4;
    if (!ctx().stmt_list.empty() && ctx().stmt_list.back() == body) ctx().stmt_list.pop_back();
    ctx().stmt_result = adopt_node_position(new pascal_s::WhileStatementNode($2, body), $2);
};

for_stmt: FOR IDENTIFIER ASSIGN expr TO expr DO stmt {
    auto* body = $<stmt>8;
    if (!ctx().stmt_list.empty() && ctx().stmt_list.back() == body) ctx().stmt_list.pop_back();
    ctx().stmt_result = set_node_position(new pascal_s::ForStatementNode($2, $4, $6, body, false));
    free($2);
}
| FOR IDENTIFIER ASSIGN expr DOWNTO expr DO stmt {
    auto* body = $<stmt>8;
    if (!ctx().stmt_list.empty() && ctx().stmt_list.back() == body) ctx().stmt_list.pop_back();
    ctx().stmt_result = set_node_position(new pascal_s::ForStatementNode($2, $4, $6, body, true));
    free($2);
};

proc_call: IDENTIFIER LPAREN { ctx().arg_list_stack.push_back({}); } arg_lst RPAREN {
    auto* c = set_node_position(new pascal_s::ProcedureCallNode($1));
    for (auto* a : ctx().arg_list_stack.back()) c->add_argument(std::unique_ptr<pascal_s::ExpressionNode>(a));
    ctx().arg_list_stack.pop_back();
    ctx().stmt_result = c;
    free($1);
}
| IDENTIFIER LPAREN RPAREN {
    ctx().stmt_result = set_node_position(new pascal_s::ProcedureCallNode($1));
    free($1);
}
| IDENTIFIER {
    ctx().stmt_result = set_node_position(new pascal_s::ProcedureCallNode($1));
    free($1);
};

write_stmt: WRITE LPAREN { ctx().arg_list_stack.push_back({}); } expr_lst RPAREN {
    auto* ws = set_node_position(new pascal_s::WriteStatementNode(nullptr));
    for (auto* e : ctx().arg_list_stack.back()) {
        ws->values.push_back(std::unique_ptr<pascal_s::ExpressionNode>(e));
    }
    ctx().arg_list_stack.pop_back();
    ctx().stmt_result = ws;
}
| WRITELN LPAREN { ctx().arg_list_stack.push_back({}); } expr_lst RPAREN {
    auto* ws = set_node_position(new pascal_s::WriteStatementNode(nullptr));
    for (auto* e : ctx().arg_list_stack.back()) {
        ws->values.push_back(std::unique_ptr<pascal_s::ExpressionNode>(e));
    }
    ctx().arg_list_stack.pop_back();
    ctx().stmt_result = ws;
}
| WRITE LPAREN RPAREN {
    ctx().stmt_result = set_node_position(new pascal_s::WriteStatementNode(nullptr));
}
| WRITELN LPAREN RPAREN {
    ctx().stmt_result = set_node_position(new pascal_s::WriteStatementNode(nullptr));
};

expr_lst: expr_lst COMMA expr { ctx().arg_list_stack.back().push_back($3); }
        | expr { ctx().arg_list_stack.back().push_back($1); };

expr: simple_expr { $$ = $1; }
| simple_expr RELOP simple_expr {
    $$ = adopt_node_position(
        new pascal_s::BinaryExpressionNode(
            relop_to_binop($2),
            std::unique_ptr<pascal_s::ExpressionNode>($1),
            std::unique_ptr<pascal_s::ExpressionNode>($3)),
        $1);
    free($2);
};

simple_expr: term { $$ = $1; }
| simple_expr ADDOP term {
    pascal_s::BinaryOp op = strcmp($2, "+") == 0
        ? pascal_s::BinaryOp::OP_ADD
        : pascal_s::BinaryOp::OP_SUB;
    $$ = adopt_node_position(
        new pascal_s::BinaryExpressionNode(
            op,
            std::unique_ptr<pascal_s::ExpressionNode>($1),
            std::unique_ptr<pascal_s::ExpressionNode>($3)),
        $1);
    free($2);
}
| simple_expr OR term {
    $$ = adopt_node_position(
        new pascal_s::BinaryExpressionNode(
            pascal_s::BinaryOp::OP_OR,
            std::unique_ptr<pascal_s::ExpressionNode>($1),
            std::unique_ptr<pascal_s::ExpressionNode>($3)),
        $1);
};

term: factor { $$ = $1; }
| term MULOP factor {
    pascal_s::BinaryOp op = strcmp($2, "*") == 0
        ? pascal_s::BinaryOp::OP_MUL
        : pascal_s::BinaryOp::OP_DIV_REAL;
    $$ = adopt_node_position(
        new pascal_s::BinaryExpressionNode(
            op,
            std::unique_ptr<pascal_s::ExpressionNode>($1),
            std::unique_ptr<pascal_s::ExpressionNode>($3)),
        $1);
    free($2);
}
| term DIV factor {
    $$ = adopt_node_position(
        new pascal_s::BinaryExpressionNode(
            pascal_s::BinaryOp::OP_DIV,
            std::unique_ptr<pascal_s::ExpressionNode>($1),
            std::unique_ptr<pascal_s::ExpressionNode>($3)),
        $1);
}
| term MOD factor {
    $$ = adopt_node_position(
        new pascal_s::BinaryExpressionNode(
            pascal_s::BinaryOp::OP_MOD,
            std::unique_ptr<pascal_s::ExpressionNode>($1),
            std::unique_ptr<pascal_s::ExpressionNode>($3)),
        $1);
}
| term AND factor {
    $$ = adopt_node_position(
        new pascal_s::BinaryExpressionNode(
            pascal_s::BinaryOp::OP_AND,
            std::unique_ptr<pascal_s::ExpressionNode>($1),
            std::unique_ptr<pascal_s::ExpressionNode>($3)),
        $1);
};

factor: INTEGER_LITERAL { $$ = set_node_position(new pascal_s::IntegerLiteralNode($1)); }
| REAL_LITERAL { $$ = set_node_position(new pascal_s::RealLiteralNode($1)); }
| BOOLEAN_LITERAL { $$ = set_node_position(new pascal_s::BooleanLiteralNode($1 != 0)); }
| CHAR_LITERAL { $$ = set_node_position(new pascal_s::CharLiteralNode($1)); }
| STRING_LITERAL { $$ = set_node_position(new pascal_s::StringLiteralNode($1)); free($1); }
| IDENTIFIER LPAREN { ctx().arg_list_stack.push_back({}); } arg_lst RPAREN {
    auto* c = set_node_position(new pascal_s::FunctionCallNode($1));
    for (auto* a : ctx().arg_list_stack.back()) c->add_argument(std::unique_ptr<pascal_s::ExpressionNode>(a));
    ctx().arg_list_stack.pop_back(); $$ = c; free($1);
}
| IDENTIFIER LPAREN RPAREN {
    $$ = set_node_position(new pascal_s::FunctionCallNode($1)); free($1);
}
| variable_ref { $$ = $1; }
| LPAREN expr RPAREN { $$ = $2; }
| NOT factor {
    $$ = adopt_node_position(
        new pascal_s::UnaryExpressionNode(
            pascal_s::UnaryOp::UOP_NOT,
            std::unique_ptr<pascal_s::ExpressionNode>($2)),
        $2);
}
| ADDOP factor {
    if (strcmp($1, "-") == 0) {
        $$ = adopt_node_position(
            new pascal_s::UnaryExpressionNode(
                pascal_s::UnaryOp::UOP_NEGATE,
                std::unique_ptr<pascal_s::ExpressionNode>($2)),
            $2);
    } else {
        $$ = $2;
    }
    free($1);
};

variable_ref: IDENTIFIER {
    $$ = set_node_position(new pascal_s::IdentifierNode($1));
    free($1);
}
| IDENTIFIER LBRACKET index_lst RBRACKET {
    auto* node = set_node_position(new pascal_s::ArrayAccessNode($1));
    for (auto* idx : *$3) {
        node->add_index(std::unique_ptr<pascal_s::ExpressionNode>(idx));
    }
    delete $3;
    $$ = node;
    free($1);
}
| variable_ref DOT IDENTIFIER {
    $$ = adopt_node_position(new pascal_s::RecordAccessNode($1, $3), $1);
    free($3);
};

arg_lst: arg_lst COMMA expr { ctx().arg_list_stack.back().push_back($3); } | expr { ctx().arg_list_stack.back().push_back($1); };

index_lst: index_lst COMMA expr { $1->push_back($3); $$ = $1; }
         | expr { $$ = new std::vector<pascal_s::ExpressionNode*>(); $$->push_back($1); };

%%

void yyerror(const char* m) {
    pascal_s::ErrorHandler::instance().syntax_error(m, yylineno, yycolumn_start, std::max(yyleng, 1));
}

