#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

#include "ast.h"
#include "symbol_table.h"
#include "error.h"
#include "codegen.h"

extern FILE* yyin;
extern int yyparse();


// 全局变量定义
pascal_s::ProgramNode* root_ast = nullptr;
pascal_s::SymbolTable g_symbol_table;

std::string derive_output_path(const std::string& input_path) {
    // 将 .pas 替换为 .c
    std::string out = input_path;
    size_t dot = out.rfind('.');
    if (dot != std::string::npos) {
        out = out.substr(0, dot) + ".c";
    } else {
        out += ".c";
    }
    return out;
}

void write_file(const std::string& fn, const std::string& content) {
    std::ofstream f(fn);
    if (!f) throw std::runtime_error("Cannot write: " + fn);
    f << content;
}

int main(int argc, char* argv[]) {
    std::string input;
    
    // 解析命令行参数: pascc -i filename.pas
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-i") && i+1 < argc) {
            input = argv[++i];
        } else if (!strcmp(argv[i], "-o") && i+1 < argc) {
            i++; // skip -o arg
        } else if (argv[i][0] != '-') {
            input = argv[i];
        }
    }
    
    if (input.empty()) {
        std::cerr << "Usage: pascc -i <input.pas>" << std::endl;
        return 1;
    }
    
    std::string output = derive_output_path(input);
    
    root_ast = nullptr;
    g_symbol_table.clear();
    pascal_s::ErrorHandler::instance().clear();
    
    FILE* in = fopen(input.c_str(), "r");
    if (!in) {
        std::cerr << "Cannot open: " << input << std::endl;
        return 1;
    }
    
    yyin = in;
    int r = yyparse();
    fclose(in);
    
    if (r != 0 || pascal_s::ErrorHandler::instance().has_errors()) {
        pascal_s::ErrorHandler::instance().print_errors();
        return 1;
    }
    
    if (!root_ast) {
        std::cerr << "No AST generated" << std::endl;
        return 1;
    }
    
    pascal_s::CodeGenerator cg;
    std::string c = cg.generate(root_ast);
    write_file(output, c);
    

    
    delete root_ast;
    root_ast = nullptr;
    
    return 0;
}
