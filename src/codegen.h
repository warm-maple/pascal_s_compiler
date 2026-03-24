#pragma once
#include "ast.h"
#include "symbol_table.h"
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

extern pascal_s::SymbolTable g_symbol_table;

namespace pascal_s {

class CodeGenerator : public ASTVisitor {
public:
    CodeGenerator() = default;
    
    std::string generate(ProgramNode* program);
    
    void visit(IntegerLiteralNode& n) override;
    void visit(RealLiteralNode& n) override;
    void visit(CharLiteralNode& n) override;
    void visit(StringLiteralNode& n) override;
    void visit(IdentifierNode& n) override;
    void visit(ArrayAccessNode& n) override;
    void visit(BinaryExpressionNode& n) override;
    void visit(UnaryExpressionNode& n) override;
    void visit(FunctionCallNode& n) override;
    void visit(AssignmentNode& n) override;
    void visit(CompoundStatementNode& n) override;
    void visit(IfStatementNode& n) override;
    void visit(WhileStatementNode& n) override;
    void visit(ForStatementNode& n) override;
    void visit(ProcedureCallNode& n) override;
    void visit(WriteStatementNode& n) override;
    void visit(VariableDeclarationNode& n) override;
    void visit(FunctionDeclarationNode& n) override;
    void visit(ProgramNode& n) override;
    
private:
    std::ostringstream output;
    std::vector<std::string> forward_declarations;
    int indent_level = 0;
    std::string current_func_name;
    std::unordered_map<std::string, DataType> var_types;
    std::unordered_map<std::string, std::vector<VariableDeclarationNode*>> func_local_vars;
    std::unordered_map<std::string, std::vector<ParameterInfo>> func_params;
    std::unordered_set<std::string> ref_params;
    
    void indent();
    std::string c_operator(BinaryOp op);
    std::string c_type(DataType t);
    std::string c_format_specifier(DataType t);
    void generate_forward_declarations();
    bool is_ref_param(const std::string& name) { return ref_params.count(name) > 0; }
};

// ============ 工具函数 ============

inline std::string CodeGenerator::c_operator(BinaryOp op) {
    switch (op) {
        case BinaryOp::OP_ADD: return "+";
        case BinaryOp::OP_SUB: return "-";
        case BinaryOp::OP_MUL: return "*";
        case BinaryOp::OP_DIV: return "/";
        case BinaryOp::OP_MOD: return "%";
        case BinaryOp::OP_AND: return "&&";
        case BinaryOp::OP_OR: return "||";
        case BinaryOp::OP_EQ: return "==";
        case BinaryOp::OP_NE: return "!=";
        case BinaryOp::OP_LT: return "<";
        case BinaryOp::OP_LE: return "<=";
        case BinaryOp::OP_GT: return ">";
        case BinaryOp::OP_GE: return ">=";
        default: return "?";
    }
}

inline std::string CodeGenerator::c_type(DataType t) {
    return TypeSystem::to_c_type(t);
}

inline std::string CodeGenerator::c_format_specifier(DataType t) {
    switch (t) {
        case DataType::TY_INTEGER: return "%d";
        case DataType::TY_REAL: return "%lf";
        case DataType::TY_CHAR: return "%c";
        default: return "%d";
    }
}

inline void CodeGenerator::indent() {
    for (int i = 0; i < indent_level; i++)
        output << "    ";
}

// ============ 主生成函数 ============

inline std::string CodeGenerator::generate(ProgramNode* program) {
    output.str("");
    output.clear();
    forward_declarations.clear();
    indent_level = 0;
    var_types.clear();
    
    output << "#include <stdio.h>\n";
    output << "#include <stdlib.h>\n";
    output << "#include <math.h>\n";
    output << "#define true 1\n";
    output << "#define false 0\n\n";
    
    program->accept(*this);
    
    // 拼接前向声明和代码体
    std::string body = output.str();
    output.str("");
    output << "#include <stdio.h>\n#include <stdlib.h>\n#include <math.h>\n";
    output << "#define true 1\n#define false 0\n\n";
    generate_forward_declarations();
    // 去掉 body 中的头部（已重复输出）
    size_t pos = body.find("#define false 0");
    if (pos != std::string::npos) {
        pos = body.find('\n', pos);
        if (pos != std::string::npos) body = body.substr(pos + 1);
    }
    output << body;
    return output.str();
}

inline void CodeGenerator::generate_forward_declarations() {
    for (const auto& decl : forward_declarations) {
        output << decl << ";\n";
    }
    if (!forward_declarations.empty()) output << "\n";
}

// ============ 表达式节点 ============

inline void CodeGenerator::visit(IntegerLiteralNode& n) {
    output << n.value;
}

inline void CodeGenerator::visit(RealLiteralNode& n) {
    output.precision(15);
    output << n.value;
    // TODO: 确保浮点数输出包含小数点
}

inline void CodeGenerator::visit(CharLiteralNode& n) {
    output << "'" << n.value << "'";
}

inline void CodeGenerator::visit(StringLiteralNode& n) {
    output << "\"" << n.value << "\"";
}

inline void CodeGenerator::visit(IdentifierNode& n) {
    if (is_ref_param(n.name)) {
        output << "*" << n.name;
    } else {
        output << n.name;
    }
}

inline void CodeGenerator::visit(ArrayAccessNode& n) {
    output << n.array_name;
    for (const auto& idx : n.indices) {
        output << "[";
        idx->accept(*this);
        output << "]";
    }
}

inline void CodeGenerator::visit(BinaryExpressionNode& n) {
    // TODO: 处理运算符优先级和括号
    output << "(";
    n.left->accept(*this);
    output << " " << c_operator(n.op) << " ";
    n.right->accept(*this);
    output << ")";
}

inline void CodeGenerator::visit(UnaryExpressionNode& n) {
    if (n.op == UnaryOp::UOP_NOT) {
        output << "!";
    } else {
        output << "-";
    }
    output << " ";
    if (n.operand) {
        n.operand->accept(*this);
    }
}

inline void CodeGenerator::visit(FunctionCallNode& n) {
    // TODO: 处理副作用参数的求值顺序
    // TODO: 处理 var 参数（引用传递）
    output << (n.func_name == "main" ? "_pascal_main" : n.func_name) << "(";
    bool first = true;
    for (auto& arg : n.arguments) {
        if (!first) output << ", ";
        first = false;
        arg->accept(*this);
    }
    output << ")";
}

// ============ 语句节点 ============

inline void CodeGenerator::visit(AssignmentNode& n) {
    indent();
    // 函数返回值赋值: funcname := value -> return value;
    if (!current_func_name.empty()) {
        if (auto* target = dynamic_cast<IdentifierNode*>(n.target.get())) {
            if (target->name == current_func_name) {
                output << "return ";
                n.value->accept(*this);
                output << ";\n";
                return;
            }
        }
    }
    n.target->accept(*this);
    output << " = ";
    n.value->accept(*this);
    output << ";\n";
}

inline void CodeGenerator::visit(CompoundStatementNode& n) {
    output << "{\n";
    indent_level++;
    for (const auto& stmt : n.statements) {
        if (stmt) stmt->accept(*this);
    }
    indent_level--;
    indent();
    output << "}\n";
}

inline void CodeGenerator::visit(IfStatementNode& n) {
    indent();
    output << "if (";
    n.condition->accept(*this);
    output << ") ";
    
    if (n.then_branch) {
        n.then_branch->accept(*this);
    }
    
    if (n.else_branch) {
        // TODO: else 花括号处理
        indent();
        output << " else ";
        n.else_branch->accept(*this);
    }
}

inline void CodeGenerator::visit(WhileStatementNode& n) {
    indent();
    output << "while (";
    n.condition->accept(*this);
    output << ") ";
    if (n.body) {
        n.body->accept(*this);
    }
}

inline void CodeGenerator::visit(ForStatementNode& n) {
    indent();
    std::string var = n.loop_var;
    output << "for (" << var << " = ";
    n.start->accept(*this);
    output << "; " << var;
    if (n.is_downto) {
        output << " >= ";
    } else {
        output << " <= ";
    }
    n.end->accept(*this);
    output << "; " << var;
    if (n.is_downto) {
        output << "--";
    } else {
        output << "++";
    }
    output << ") ";
    if (n.body) {
        n.body->accept(*this);
    }
}

inline void CodeGenerator::visit(ProcedureCallNode& n) {
    indent();
    std::string name = n.proc_name;
    
    // write / writeln
    if (name == "write" || name == "writeln") {
        for (auto& arg : n.arguments) {
            indent();
            // TODO: 根据表达式类型选择正确的格式符
            // 简单处理：先统一用 %d
            output << "printf(\"%d\", ";
            arg->accept(*this);
            output << ");\n";
        }
        if (name == "writeln") {
            indent();
            output << "printf(\"\\n\");\n";
        }
        return;
    }
    
    // read / readln
    if (name == "read" || name == "readln") {
        for (auto& arg : n.arguments) {
            indent();
            // TODO: 根据变量类型选择格式符
            output << "scanf(\"%d\", &";
            arg->accept(*this);
            output << ");\n";
        }
        return;
    }
    
    // 普通过程调用
    // TODO: 处理 var 参数（引用传递）
    output << name << "(";
    bool first = true;
    for (auto& arg : n.arguments) {
        if (!first) output << ", ";
        first = false;
        arg->accept(*this);
    }
    output << ");\n";
}

// 注意: WriteStatementNode 在当前 parser 中没有用到
inline void CodeGenerator::visit(WriteStatementNode& n) {
    // TODO: 由 ProcedureCallNode 的 write/writeln 处理
}

// ============ 声明节点 ============

inline void CodeGenerator::visit(VariableDeclarationNode& n) {
    var_types[n.var_name] = n.type;
    
    if (n.is_const) {
        // 常量
        output << "const " << c_type(n.type) << " " << n.var_name << " = ";
        if (n.init_value) {
            n.init_value->accept(*this);
        }
        output << ";\n";
    } else if (n.type == DataType::TY_ARRAY || n.is_array) {
        // 数组
        std::string elem = "int";
        if (n.array_info.element_type != DataType::TY_UNKNOWN) {
            elem = c_type(n.array_info.element_type);
        }
        output << elem << " " << n.var_name;
        if (!n.array_info.dimensions.empty()) {
            for (const auto& dim : n.array_info.dimensions) {
                output << "[" << dim.size() << "]";
            }
        } else if (n.array_info.upper_bound >= n.array_info.lower_bound) {
            int size = n.array_info.upper_bound - n.array_info.lower_bound + 1;
            output << "[" << size << "]";
        }
        output << ";\n";
    } else {
        // 普通变量
        output << c_type(n.type) << " " << n.var_name << ";\n";
    }
}

inline void CodeGenerator::visit(FunctionDeclarationNode& n) {
    // 收集参数和局部变量信息
    std::vector<ParameterInfo> params = n.parameters;
    func_params[n.func_name] = params;
    
    // 生成前向声明
    std::ostringstream fwd;
    fwd << c_type(n.return_type) << " " << n.func_name << "(";
    bool first = true;
    for (const auto& p : params) {
        if (!first) fwd << ", ";
        first = false;
        if (p.is_reference) {
            fwd << c_type(p.type) << "* " << p.name;
        } else {
            fwd << c_type(p.type) << " " << p.name;
        }
    }
    fwd << ")";
    forward_declarations.push_back(fwd.str());
    
    // 生成函数定义
    output << fwd.str() << " {\n";
    
    std::string prev_func = current_func_name;
    current_func_name = n.func_name;
    
    ref_params.clear();
    for (const auto& p : params) {
        if (p.is_reference) ref_params.insert(p.name);
        var_types[p.name] = p.type;
    }
    
    indent_level++;
    
    // 局部变量
    auto it = func_local_vars.find(n.func_name);
    if (it != func_local_vars.end()) {
        for (auto* var : it->second) {
            var_types[var->var_name] = var->type;
            indent();
            if (var->type == DataType::TY_ARRAY || var->is_array) {
                // TODO: 多维数组局部变量
                std::string elem = "int";
                if (var->array_info.element_type != DataType::TY_UNKNOWN)
                    elem = c_type(var->array_info.element_type);
                output << elem << " " << var->var_name;
                if (var->array_info.upper_bound >= var->array_info.lower_bound) {
                    int sz = var->array_info.upper_bound - var->array_info.lower_bound + 1;
                    output << "[" << sz << "]";
                }
                output << ";\n";
            } else {
                output << c_type(var->type) << " " << var->var_name << ";\n";
            }
        }
    }
    
    // 函数体
    if (n.body) {
        if (auto* compound = dynamic_cast<CompoundStatementNode*>(n.body.get())) {
            for (const auto& stmt : compound->statements) {
                if (stmt) stmt->accept(*this);
            }
        } else {
            n.body->accept(*this);
        }
    }
    
    indent_level--;
    current_func_name = prev_func;
    output << "}\n\n";
}

inline void CodeGenerator::visit(ProgramNode& n) {
    // 输出全局变量和常量
    for (const auto& decl : n.declarations) {
        if (auto* var = dynamic_cast<VariableDeclarationNode*>(decl.get())) {
            var->accept(*this);
        }
    }
    output << "\n";
    
    // 输出函数定义（局部变量从 FunctionDeclarationNode.local_vars 获取）
    for (const auto& decl : n.declarations) {
        if (auto* func = dynamic_cast<FunctionDeclarationNode*>(decl.get())) {
            // 收集局部变量到 func_local_vars
            std::vector<VariableDeclarationNode*> locals;
            for (auto& lv : func->local_vars) {
                locals.push_back(lv.get());
            }
            func_local_vars[func->func_name] = locals;
            func->accept(*this);
        }
    }
    
    // 主程序
    output << "int main() {\n";
    indent_level++;
    if (n.main_body) {
        for (const auto& stmt : n.main_body->statements) {
            if (stmt) stmt->accept(*this);
        }
    }
    indent();
    output << "return 0;\n";
    indent_level--;
    output << "}\n";
}

} // namespace pascal_s
