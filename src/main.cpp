#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

#include "ast.h"
#include "symbol_table.h"
#include "error.h"
#include "parser_state.h"
#include "semantic_analyzer.h"
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
    std::string output;
    
    // 解析命令行参数: pascc -i filename.pas
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-i") && i+1 < argc) {
            input = argv[++i];
        } else if (!strcmp(argv[i], "-o") && i+1 < argc) {
            output = argv[++i];
        } else if (argv[i][0] != '-') {
            input = argv[i];
        }
    }
    
    if (input.empty()) {
        std::cerr << "Usage: pascc -i <input.pas>" << std::endl;
        return 1;
    }
    
    if (output.empty()) {
        output = derive_output_path(input);
    }
    
    root_ast = nullptr;
    g_symbol_table.clear();
    pascal_s::reset_parser_state();
    pascal_s::ErrorHandler::instance().clear();
    
    // 预读源码文本，后续 `caret diagnostics` 需要直接回显原始代码行并画出定位箭头。
    std::vector<std::string> source_lines;
    {
        std::ifstream src_file(input);
        if (src_file) {
            std::string line;
            while (std::getline(src_file, line)) {
                source_lines.push_back(line);
            }
        }
    }
    pascal_s::ErrorHandler::instance().set_source_lines(source_lines);
    
    FILE* in = fopen(input.c_str(), "r");
    if (!in) {
        std::cerr << "Cannot open: " << input << std::endl;
        return 1;
    }
    
    yyin = in;
    int r = yyparse();
    fclose(in);
    
    // 语法阶段一旦留下错误，后续不再进入 semantic/codegen，避免在半截 AST 上继续传播问题。
    if (r != 0 || pascal_s::ErrorHandler::instance().has_errors()) {
        pascal_s::ErrorHandler::instance().print_errors();
        return 1;
    }
    
    if (!root_ast) {
        std::cerr << "No AST generated" << std::endl;
        return 1;
    }
    
    // 主控流水线：parse -> semantic -> codegen。每一阶段都在前一阶段“无错误”的前提下运行。
    pascal_s::SemanticAnalyzer analyzer;
    root_ast->accept(analyzer);
    g_symbol_table = analyzer.sym_table;
    
    if (pascal_s::ErrorHandler::instance().has_errors()) {
        pascal_s::ErrorHandler::instance().print_errors();
        return 1;
    }
    
    // 代码生成阶段只接收已经通过语义检查的 AST，保证输出的 C 源码是可信结果。
    pascal_s::CodeGenerator cg;
    std::string c = cg.generate(root_ast);
    write_file(output, c);
    

    
    delete root_ast;
    root_ast = nullptr;
    
    return 0;
}
