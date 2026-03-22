#include "src/ast.h"
#include "src/symbol_table.h"
#include <iostream>

extern pascal_s::ProgramNode* root_ast;
extern pascal_s::SymbolTable g_symbol_table;
extern int yyparse();
extern FILE* yyin;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.pas>" << std::endl;
        return 1;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        std::cerr << "Cannot open " << argv[1] << std::endl;
        return 1;
    }
    yyparse();
    
    std::cout << "=== Symbol Table ===" << std::endl;
    // 打印所有符号
    // 注意：符号表是私有的，我们需要通过 lookup 来访问
    
    // 打印 AST 声明
    std::cout << "=== AST Declarations ===" << std::endl;
    for (const auto& decl : root_ast->declarations) {
        if (auto* var = dynamic_cast<pascal_s::VariableDeclarationNode*>(decl.get())) {
            auto sym = g_symbol_table.lookup(var->var_name);
            std::cout << "Var: " << var->var_name 
                      << ", scope_level: " << (sym ? sym->scope_level : -1)
                      << std::endl;
        } else if (auto* func = dynamic_cast<pascal_s::FunctionDeclarationNode*>(decl.get())) {
            std::cout << "Func: " << func->func_name << std::endl;
        }
    }
    
    return 0;
}
