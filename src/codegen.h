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
    void visit(BooleanLiteralNode& n) override;
    void visit(CharLiteralNode& n) override;
    void visit(StringLiteralNode& n) override;
    void visit(IdentifierNode& n) override;
    void visit(ArrayAccessNode& n) override;
    void visit(RecordAccessNode& n) override;
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
    std::vector<std::string> record_definitions;
    std::vector<std::string> forward_declarations;  // 函数前向声明
    int indent_level = 0;
    std::string current_func_name;  // 当前函数名（用于处理函数返回值）
    std::unordered_map<std::string, DataType> var_types;
    std::unordered_set<std::string> string_consts;  // 多字符字符串常量集合
    std::unordered_map<std::string, std::vector<VariableDeclarationNode*>> func_local_vars;  // 函数局部变量
    std::unordered_map<std::string, std::vector<ParameterInfo>> func_params;  // 函数参数信息
    std::unordered_set<std::string> ref_params;  // 当前函数中的引用参数（var 参数）
    int temp_var_counter = 0;  // 临时变量计数器（用于副作用参数）
    
    void indent();
    std::string c_operator(BinaryOp op);
    std::string c_type(DataType t);
    std::string c_decl_type(DataType t, const RecordInfo* record_info = nullptr);
    std::string c_format_specifier(DataType t, bool for_scanf = false);
    void generate_record_definitions();
    void generate_forward_declarations();
    void collect_record_definition(const RecordInfo& record_info);
    DataType get_identifier_type(const std::string& name);
    bool is_ref_param(const std::string& name);  // 检查是否是引用参数
    void generate_expression(ExpressionNode* expr, bool is_arg = false, int arg_index = -1, const std::string& func_name = "");
    DataType get_expr_type(ExpressionNode* expr);  // 获取表达式类型
};

} // namespace pascal_s
