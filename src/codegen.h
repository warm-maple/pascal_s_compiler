#pragma once
#include "ast.h"
#include "symbol_table.h"
#include <string>
#include <sstream>
#include <vector>

namespace pascal_s {

class CodeGenerator : public ASTVisitor {
public:
    CodeGenerator() = default;
    
    // 生成 C 代码
    std::string generate(ProgramNode* program);
    
    // 访问者方法
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
    std::vector<std::string> forward_declarations;  // 函数前向声明
    int indent_level = 0;
    std::string current_func_name;  // 当前函数名（用于处理函数返回值）
    
    void indent();
    std::string c_operator(BinaryOp op);
    std::string c_type(DataType t);
    std::string c_format_specifier(DataType t);
    void generate_forward_declarations();
};

// 工具函数实现
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
        case DataType::TY_REAL: return "%g";
        case DataType::TY_BOOLEAN: return "%d";
        case DataType::TY_CHAR: return "%c";
        default: return "%d";
    }
}

inline void CodeGenerator::indent() {
    for (int i = 0; i < indent_level; i++) {
        output << "    ";
    }
}

inline void CodeGenerator::generate_forward_declarations() {
    for (const auto& decl : forward_declarations) {
        output << decl << ";\n";
    }
    if (!forward_declarations.empty()) {
        output << "\n";
    }
}

// 主生成函数
inline std::string CodeGenerator::generate(ProgramNode* program) {
    output.str("");
    output.clear();
    forward_declarations.clear();
    indent_level = 0;
    
    // C 头文件
    output << "#include <stdio.h>\n";
    output << "#include <stdlib.h>\n\n";
    
    // 收集函数前向声明
    for (const auto& decl : program->declarations) {
        if (auto* func = dynamic_cast<FunctionDeclarationNode*>(decl.get())) {
            std::ostringstream ss;
            ss << c_type(func->return_type) << " " << func->func_name << "(";
            bool first = true;
            for (const auto& param : func->parameters) {
                if (!first) ss << ", ";
                first = false;
                if (param.is_reference) {
                    ss << c_type(param.type) << "* " << param.name;
                } else {
                    ss << c_type(param.type) << " " << param.name;
                }
            }
            ss << ")";
            forward_declarations.push_back(ss.str());
        }
    }
    
    // 输出前向声明
    generate_forward_declarations();
    
    // 先生成全局变量声明（在 main 外部）
    for (const auto& decl : program->declarations) {
        if (auto* var = dynamic_cast<VariableDeclarationNode*>(decl.get())) {
            decl->accept(*this);
        }
    }
    if (!program->declarations.empty()) {
        // 检查是否有变量声明
        bool has_vars = false;
        for (const auto& decl : program->declarations) {
            if (dynamic_cast<VariableDeclarationNode*>(decl.get())) {
                has_vars = true;
                break;
            }
        }
        if (has_vars) output << "\n";
    }
    
    // 处理函数定义（在 main 外部）
    for (const auto& decl : program->declarations) {
        if (auto* func = dynamic_cast<FunctionDeclarationNode*>(decl.get())) {
            decl->accept(*this);
        }
    }
    
    // 主函数
    output << "int main() {\n";
    indent_level++;
    
    // 主函数体 - 直接生成语句，不再生成额外的花括号
    if (program->main_body) {
        // 直接处理复合语句中的每个语句
        for (const auto& stmt : program->main_body->statements) {
            if (stmt) stmt->accept(*this);
        }
    }
    
    // 默认返回值
    indent();
    output << "return 0;\n";
    
    indent_level--;
    output << "}\n";
    
    return output.str();
}

// 访问者实现
inline void CodeGenerator::visit(IntegerLiteralNode& n) {
    output << n.value;
}

inline void CodeGenerator::visit(RealLiteralNode& n) {
    output << n.value;
}

inline void CodeGenerator::visit(CharLiteralNode& n) {
    output << "'" << n.value << "'";
}

inline void CodeGenerator::visit(StringLiteralNode& n) {
    output << "\"" << n.value << "\"";
}

inline void CodeGenerator::visit(IdentifierNode& n) {
    output << n.name;
}

inline void CodeGenerator::visit(ArrayAccessNode& n) {
    output << n.array_name << "[";
    for (size_t i = 0; i < n.indices.size(); i++) {
        if (i > 0) output << ", ";
        n.indices[i]->accept(*this);
    }
    output << "]";
}

inline void CodeGenerator::visit(BinaryExpressionNode& n) {
    output << "(";
    n.left->accept(*this);
    output << " " << c_operator(n.op) << " ";
    n.right->accept(*this);
    output << ")";
}

inline void CodeGenerator::visit(UnaryExpressionNode& n) {
    if (n.op == UnaryOp::UOP_NOT) {
        output << "!";
    } else if (n.op == UnaryOp::UOP_NEGATE) {
        output << "-";
    }
    output << "(";
    if (n.operand) {
        n.operand->accept(*this);
    } else {
        output << "0";
    }
    output << ")";
}

inline void CodeGenerator::visit(FunctionCallNode& n) {
    output << n.func_name << "(";
    bool first = true;
    for (const auto& arg : n.arguments) {
        if (!first) output << ", ";
        first = false;
        arg->accept(*this);
    }
    output << ")";
}

inline void CodeGenerator::visit(AssignmentNode& n) {
    indent();
    // 检查是否是函数返回值赋值（func := value）
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
    // 只在不是函数体的情况下生成花括号
    // 函数体的花括号已经在 visit(FunctionDeclarationNode) 中生成
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
    output << ") {\n";
    indent_level++;
    if (n.then_branch) {
        n.then_branch->accept(*this);
    }
    indent_level--;
    if (n.else_branch) {
        indent();
        output << "} else {\n";
        indent_level++;
        n.else_branch->accept(*this);
        indent_level--;
    }
    indent();
    output << "}\n";
}

inline void CodeGenerator::visit(WhileStatementNode& n) {
    indent();
    output << "while (";
    n.condition->accept(*this);
    output << ") {\n";
    indent_level++;
    n.body->accept(*this);
    indent_level--;
    indent();
    output << "}\n";
}

inline void CodeGenerator::visit(ForStatementNode& n) {
    indent();
    output << "for (" << c_type(DataType::TY_INTEGER) << " " << n.loop_var << " = ";
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

inline void CodeGenerator::visit(ProcedureCallNode& n) {
    indent();
    output << n.proc_name << "(";
    bool first = true;
    for (const auto& arg : n.arguments) {
        if (!first) output << ", ";
        first = false;
        arg->accept(*this);
    }
    output << ");\n";
}

inline void CodeGenerator::visit(WriteStatementNode& n) {
    indent();
    // 支持多个值的 write 语句
    if (!n.values.empty()) {
        // 多个值的情况
        output << "printf(\"";
        for (size_t i = 0; i < n.values.size(); i++) {
            DataType val_type = DataType::TY_INTEGER;
            if (dynamic_cast<RealLiteralNode*>(n.values[i].get())) {
                val_type = DataType::TY_REAL;
            } else if (dynamic_cast<StringLiteralNode*>(n.values[i].get())) {
                val_type = DataType::TY_CHAR;  // 字符串用 %s
            }
            if (i > 0) output << " ";  // 值之间加空格
            output << c_format_specifier(val_type);
        }
        output << "\", ";
        for (size_t i = 0; i < n.values.size(); i++) {
            if (i > 0) output << ", ";
            n.values[i]->accept(*this);
        }
        output << ");\n";
    } else if (n.value) {
        // 单个值的情况（向后兼容）
        DataType val_type = DataType::TY_INTEGER;
        if (dynamic_cast<RealLiteralNode*>(n.value.get())) {
            val_type = DataType::TY_REAL;
        }
        output << "printf(\"" << c_format_specifier(val_type) << "\", ";
        n.value->accept(*this);
        output << ");\n";
    } else {
        // 没有值的情况
        output << "printf(\"\\n\");\n";
    }
}

inline void CodeGenerator::visit(VariableDeclarationNode& n) {
    if (n.is_const) {
        // 常量定义
        indent();
        output << "const " << c_type(n.type) << " " << n.var_name << " = ";
        if (n.init_value) {
            n.init_value->accept(*this);
        } else {
            output << "0";
        }
        output << ";\n";
    } else {
        // 变量定义
        indent();
        if (n.type == DataType::TY_ARRAY || n.is_array) {
            // 数组类型 - 需要获取数组信息
            int size = 1;
            if (n.array_info.upper_bound >= n.array_info.lower_bound) {
                size = n.array_info.upper_bound - n.array_info.lower_bound + 1;
            }
            // 数组元素类型
            std::string elem_type = "int";  // 默认 integer
            if (n.array_info.element_type != DataType::TY_UNKNOWN) {
                elem_type = c_type(n.array_info.element_type);
            }
            output << elem_type << " " << n.var_name << "[" << size << "];\n";
        } else {
            output << c_type(n.type) << " " << n.var_name << ";\n";
        }
    }
}

inline void CodeGenerator::visit(FunctionDeclarationNode& n) {
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
    
    // 设置当前函数名（用于处理函数返回值）
    std::string prev_func_name = current_func_name;
    current_func_name = n.func_name;
    
    // 函数体 - 直接生成语句，不再生成额外的花括号
    indent_level++;
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
    
    // 恢复之前的函数名
    current_func_name = prev_func_name;
    
    output << "}\n\n";
}

inline void CodeGenerator::visit(ProgramNode& n) {
    // ProgramNode 由 generate() 函数处理
}

} // namespace pascal_s
