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

void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " [options] <input.pas>\n";
    std::cerr << "  -o <output.c>  Specify output file (default: output.c)\n";
    std::cerr << "  -h             Show this help\n";
}

void write_file(const std::string& fn, const std::string& content) {
    std::ofstream f(fn);
    if (!f) throw std::runtime_error("Cannot write: " + fn);
    f << content;
}

int main(int argc, char* argv[]) {
    std::string input, output = "output.c";
    
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o") && i+1 < argc) output = argv[++i];
        else if (!strcmp(argv[i], "-h")) { print_usage(argv[0]); return 0; }
        else if (argv[i][0] != '-') input = argv[i];
    }
    
    if (input.empty()) { std::cerr << "No input file\n"; return 1; }
    
    root_ast = nullptr;
    g_symbol_table.clear();
    pascal_s::ErrorHandler::instance().clear();
    
    std::cout << "Compiling: " << input << std::endl;
    
    FILE* in = fopen(input.c_str(), "r");
    if (!in) { std::cerr << "Cannot open: " << input << std::endl; return 1; }
    
    yyin = in;
    int r = yyparse();
    fclose(in);
    
    if (r != 0 || pascal_s::ErrorHandler::instance().has_errors()) {
        std::cerr << "Failed with " << pascal_s::ErrorHandler::instance().error_count() << " errors\n";
        pascal_s::ErrorHandler::instance().print_errors();
        return 1;
    }
    
    if (!root_ast) { std::cerr << "No AST\n"; return 1; }
    
    std::cout << "Parsing OK. Generating code..." << std::endl;
    
    pascal_s::CodeGenerator cg;
    std::cout << "DEBUG: Starting generate..." << std::endl;
    std::string c = cg.generate(root_ast);
    std::cout << "DEBUG: Generate done, c.size()=" << c.size() << std::endl;
    write_file(output, c);
    
    std::cout << "Output: " << output << std::endl;
    std::cout << "Success!" << std::endl;
    return 0;
}
