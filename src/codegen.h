#pragma once
#include "ast.h"
#include "symbol_table.h"
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// 声明全局符号表（在 parser.y 中定义）
extern pascal_s::SymbolTable g_symbol_table;

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
    std::unordered_map<std::string, DataType> var_types;  // 变量类型映射
    std::unordered_map<std::string, std::vector<VariableDeclarationNode*>> func_local_vars;  // 函数局部变量
    std::unordered_map<std::string, std::vector<ParameterInfo>> func_params;  // 函数参数信息
    std::unordered_set<std::string> ref_params;  // 当前函数中的引用参数（var 参数）
    
    void indent();
    std::string c_operator(BinaryOp op);
    std::string c_type(DataType t);
    std::string c_format_specifier(DataType t);
    void generate_forward_declarations();
    DataType get_identifier_type(const std::string& name);
    bool is_ref_param(const std::string& name);  // 检查是否是引用参数
    void generate_expression(ExpressionNode* expr, bool is_arg = false, int arg_index = -1, const std::string& func_name = "");
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
    var_types.clear();
    
    // C 头文件
    output << "#include <stdio.h>\n";
    output << "#include <stdlib.h>\n";
    output << "#define true 1\n";
    output << "#define false 0\n\n";
    
    // 收集函数前向声明
    std::vector<FunctionDeclarationNode*> funcs;
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
            funcs.push_back(func);
        }
    }
    
    // 清除之前的局部变量信息
    func_local_vars.clear();
    
    // 从函数声明中获取局部变量
    for (const auto& decl : program->declarations) {
        if (auto* func = dynamic_cast<FunctionDeclarationNode*>(decl.get())) {
            for (const auto& var : func->local_vars) {
                func_local_vars[func->func_name].push_back(var.get());
            }
        }
    }
    
    // 输出前向声明
    generate_forward_declarations();
    
    // 跟踪已声明的全局变量名
    std::unordered_set<std::string> declared_globals;
    
    // 先生成全局变量声明（在 main 外部）- program->declarations 中的 VariableDeclarationNode 都是全局变量
    for (const auto& decl : program->declarations) {
        if (auto* var = dynamic_cast<VariableDeclarationNode*>(decl.get())) {
            if (declared_globals.find(var->var_name) == declared_globals.end()) {
                decl->accept(*this);
                declared_globals.insert(var->var_name);
            }
        }
    }
    if (!declared_globals.empty()) {
        output << "\n";
    }
    
    // 处理函数定义（在 main 外部）
    for (const auto& decl : program->declarations) {
        if (auto* func = dynamic_cast<FunctionDeclarationNode*>(decl.get())) {
            decl->accept(*this);
        }
    }
    
    // 主函数 - 清除当前函数的引用参数集合，但保留 func_params 用于查找函数签名
    ref_params.clear();
    current_func_name = "";
    
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
    // 如果是引用参数，需要解引用
    if (is_ref_param(n.name)) {
        output << "*";
    }
    output << n.name;
}

inline void CodeGenerator::visit(ArrayAccessNode& n) {
    // 生成 C 风格的多维数组访问：array[i][j][k]
    output << n.array_name;
    for (size_t i = 0; i < n.indices.size(); i++) {
        output << "[";
        n.indices[i]->accept(*this);
        output << "]";
    }
}

// 获取运算符优先级（越高优先级越高）
static int get_precedence(pascal_s::BinaryOp op) {
    using namespace pascal_s;
    switch (op) {
        case BinaryOp::OP_MUL: case BinaryOp::OP_DIV: case BinaryOp::OP_MOD: case BinaryOp::OP_AND: return 3;
        case BinaryOp::OP_ADD: case BinaryOp::OP_SUB: case BinaryOp::OP_OR: return 2;
        case BinaryOp::OP_EQ: case BinaryOp::OP_NE: case BinaryOp::OP_LT: case BinaryOp::OP_LE: case BinaryOp::OP_GT: case BinaryOp::OP_GE: return 1;
        default: return 0;
    }
}

// 获取表达式的有效优先级
static int get_expr_precedence(pascal_s::ExpressionNode* expr) {
    using namespace pascal_s;
    if (auto* bin = dynamic_cast<BinaryExpressionNode*>(expr)) {
        return get_precedence(bin->op);
    }
    if (auto* un = dynamic_cast<UnaryExpressionNode*>(expr)) {
        return 4;  // 一元运算符优先级最高
    }
    return 10;  // 字面量、标识符等优先级最高
}

inline void CodeGenerator::visit(BinaryExpressionNode& n) {
    int my_prec = get_precedence(n.op);
    
    // 左子节点：只在优先级低于当前时加括号
    bool need_left_paren = get_expr_precedence(n.left.get()) < my_prec;
    if (need_left_paren) output << "(";
    n.left->accept(*this);
    if (need_left_paren) output << ")";
    
    output << " " << c_operator(n.op) << " ";
    
    // 右子节点：优先级低于或等于当前时加括号（因为右结合性）
    bool need_right_paren = get_expr_precedence(n.right.get()) <= my_prec;
    if (need_right_paren) output << "(";
    n.right->accept(*this);
    if (need_right_paren) output << ")";
}

inline void CodeGenerator::visit(UnaryExpressionNode& n) {
    if (n.op == pascal_s::UnaryOp::UOP_NOT) {
        output << "!";
    } else if (n.op == pascal_s::UnaryOp::UOP_NEGATE) {
        output << "-";
    }
    output << " ";  // 添加空格以避免 --- 被解析为递减运算符
    
    // 一元运算符的操作数通常不需要括号，除非是二元表达式
    bool need_paren = dynamic_cast<pascal_s::BinaryExpressionNode*>(n.operand.get()) != nullptr;
    if (need_paren) output << "(";
    if (n.operand) {
        n.operand->accept(*this);
    } else {
        output << "0";
    }
    if (need_paren) output << ")";
}

inline void CodeGenerator::visit(FunctionCallNode& n) {
    output << n.func_name << "(";
    bool first = true;
    // 查找函数参数信息
    auto it = func_params.find(n.func_name);
    for (size_t i = 0; i < n.arguments.size(); i++) {
        if (!first) output << ", ";
        first = false;
        
        bool is_ref = false;
        if (it != func_params.end() && i < it->second.size()) {
            is_ref = it->second[i].is_reference;
        }
        
        // 如果是引用参数，需要传递地址
        // 但如果参数本身是引用参数（指针），则直接传递
        if (is_ref) {
            auto* arg_ident = dynamic_cast<IdentifierNode*>(n.arguments[i].get());
            if (arg_ident && is_ref_param(arg_ident->name)) {
                // 参数本身是引用参数，直接传递指针（不添加 & 也不解引用）
                output << arg_ident->name;
                continue;
            } else {
                // 其他情况都需要传递地址
                output << "&";
            }
        }
        n.arguments[i]->accept(*this);
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
    // 直接生成目标，IdentifierNode 会处理引用参数的解引用
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
    output << ") ";
    
    // 如果 then_branch 是复合语句，不需要额外的花括号
    bool then_is_compound = (n.then_branch && dynamic_cast<CompoundStatementNode*>(n.then_branch.get()));
    if (!then_is_compound) {
        output << "{\n";
        indent_level++;
    }
    
    if (n.then_branch) {
        n.then_branch->accept(*this);
    }
    
    if (!then_is_compound) {
        indent_level--;
        indent();
        output << "}";
    }
    
    if (n.else_branch) {
        output << " else ";
        bool else_is_compound = dynamic_cast<CompoundStatementNode*>(n.else_branch.get());
        if (!else_is_compound) {
            output << "{\n";
            indent_level++;
        }
        n.else_branch->accept(*this);
        if (!else_is_compound) {
            indent_level--;
            indent();
            output << "}";
        }
    }
    output << "\n";
}

inline void CodeGenerator::visit(WhileStatementNode& n) {
    indent();
    output << "while (";
    n.condition->accept(*this);
    output << ") ";
    
    bool body_is_compound = (n.body && dynamic_cast<CompoundStatementNode*>(n.body.get()));
    if (!body_is_compound) {
        output << "{\n";
        indent_level++;
    }
    
    n.body->accept(*this);
    
    if (!body_is_compound) {
        indent_level--;
        indent();
        output << "}\n";
    } else {
        output << "\n";
    }
}

inline void CodeGenerator::visit(ForStatementNode& n) {
    indent();
    output << "for (" << c_type(DataType::TY_INTEGER) << " " << n.loop_var << " = ";
    n.start->accept(*this);
    output << "; " << n.loop_var << (n.is_downto ? " >= " : " <= ");
    n.end->accept(*this);
    output << "; " << n.loop_var << (n.is_downto ? "--" : "++") << ") ";
    
    bool body_is_compound = (n.body && dynamic_cast<CompoundStatementNode*>(n.body.get()));
    if (!body_is_compound) {
        output << "{\n";
        indent_level++;
    }
    
    n.body->accept(*this);
    
    if (!body_is_compound) {
        indent_level--;
        indent();
        output << "}\n";
    } else {
        output << "\n";
    }
}

inline void CodeGenerator::visit(ProcedureCallNode& n) {
    indent();
    // 特殊处理 read 过程
    if (n.proc_name == "read") {
        output << "scanf(\"%d\", &";
        if (!n.arguments.empty()) {
            n.arguments[0]->accept(*this);
        }
        output << ");\n";
    } else {
        output << n.proc_name << "(";
        bool first = true;
        // 查找过程参数信息
        auto it = func_params.find(n.proc_name);
        for (size_t i = 0; i < n.arguments.size(); i++) {
            if (!first) output << ", ";
            first = false;
            
            bool is_ref = false;
            if (it != func_params.end() && i < it->second.size()) {
                is_ref = it->second[i].is_reference;
            }
            
            // 如果是引用参数，需要传递地址
            if (is_ref) {
                auto* arg_ident = dynamic_cast<IdentifierNode*>(n.arguments[i].get());
                if (arg_ident && is_ref_param(arg_ident->name)) {
                    // 参数本身是引用参数，直接传递指针
                    output << arg_ident->name;
                    continue;
                } else {
                    // 其他情况都需要传递地址
                    output << "&";
                }
            }
            n.arguments[i]->accept(*this);
        }
        output << ");\n";
    }
}

inline void CodeGenerator::visit(WriteStatementNode& n) {
    indent();
    // 辅助函数：获取表达式类型
    auto get_expr_type = [this](const std::unique_ptr<ExpressionNode>& expr) -> DataType {
        if (dynamic_cast<RealLiteralNode*>(expr.get())) return DataType::TY_REAL;
        if (dynamic_cast<CharLiteralNode*>(expr.get())) return DataType::TY_CHAR;
        if (dynamic_cast<StringLiteralNode*>(expr.get())) return DataType::TY_CHAR;
        if (auto* id = dynamic_cast<IdentifierNode*>(expr.get())) {
            return get_identifier_type(id->name);
        }
        if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr.get())) {
            return get_identifier_type(arr->array_name);
        }
        return DataType::TY_INTEGER;
    };
    
    // 支持多个值的 write 语句
    if (!n.values.empty()) {
        // 多个值的情况
        output << "printf(\"";
        for (size_t i = 0; i < n.values.size(); i++) {
            DataType val_type = get_expr_type(n.values[i]);
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
        DataType val_type = get_expr_type(n.value);
        output << "printf(\"" << c_format_specifier(val_type) << "\", ";
        n.value->accept(*this);
        output << ");\n";
    } else {
        // 没有值的情况
        output << "printf(\"\\n\");\n";
    }
}

inline void CodeGenerator::visit(VariableDeclarationNode& n) {
    // 记录变量类型
    var_types[n.var_name] = n.type;
    
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
            // 数组类型 - 支持多维数组
            std::string elem_type = "int";  // 默认 integer
            if (n.array_info.element_type != DataType::TY_UNKNOWN) {
                elem_type = c_type(n.array_info.element_type);
            }
            
            output << elem_type << " " << n.var_name;
            
            // 生成多维数组声明
            if (!n.array_info.dimensions.empty()) {
                // 多维数组：生成 [dim1][dim2][dim3]...
                for (const auto& dim : n.array_info.dimensions) {
                    output << "[" << dim.size() << "]";
                }
            } else if (n.array_info.upper_bound >= n.array_info.lower_bound) {
                // 向后兼容：单维数组
                int size = n.array_info.upper_bound - n.array_info.lower_bound + 1;
                output << "[" << size << "]";
            } else {
                output << "[1]";  // 默认大小
            }
            output << ";\n";
        } else {
            output << c_type(n.type) << " " << n.var_name << ";\n";
        }
    }
}

inline void CodeGenerator::visit(FunctionDeclarationNode& n) {
    // 保存函数参数信息
    func_params[n.func_name] = n.parameters;
    
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
    
    // 设置当前函数的引用参数集合
    ref_params.clear();
    for (const auto& param : n.parameters) {
        if (param.is_reference) {
            ref_params.insert(param.name);
        }
    }
    
    indent_level++;
    
    // 生成局部变量声明
    auto it = func_local_vars.find(n.func_name);
    if (it != func_local_vars.end()) {
        for (auto* var : it->second) {
            // 记录变量类型
            var_types[var->var_name] = var->type;
            
            indent();
            if (var->type == DataType::TY_ARRAY || var->is_array) {
                std::string elem_type = "int";
                if (var->array_info.element_type != DataType::TY_UNKNOWN) {
                    elem_type = c_type(var->array_info.element_type);
                }
                
                output << elem_type << " " << var->var_name;
                
                // 生成多维数组声明
                if (!var->array_info.dimensions.empty()) {
                    for (const auto& dim : var->array_info.dimensions) {
                        output << "[" << dim.size() << "]";
                    }
                } else if (var->array_info.upper_bound >= var->array_info.lower_bound) {
                    int size = var->array_info.upper_bound - var->array_info.lower_bound + 1;
                    output << "[" << size << "]";
                } else {
                    output << "[1]";
                }
                output << ";\n";
            } else {
                output << c_type(var->type) << " " << var->var_name << ";\n";
            }
        }
    }
    
    // 函数体 - 直接生成语句
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

inline pascal_s::DataType CodeGenerator::get_identifier_type(const std::string& name) {
    auto it = var_types.find(name);
    if (it != var_types.end()) {
        return it->second;
    }
    return DataType::TY_INTEGER;  // 默认类型
}

inline bool CodeGenerator::is_ref_param(const std::string& name) {
    return ref_params.find(name) != ref_params.end();
}

inline void CodeGenerator::visit(ProgramNode& n) {
    // ProgramNode 由 generate() 函数处理
}

} // namespace pascal_s
