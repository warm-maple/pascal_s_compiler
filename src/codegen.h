#pragma once
#include "ast.h"
#include "symbol_table.h"
#include "type_resolver.h"
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
    std::unordered_map<std::string, SymbolTypeInfo> var_types;
    std::unordered_set<std::string> string_consts;  // 多字符字符串常量集合
    std::unordered_map<std::string, std::vector<VariableDeclarationNode*>> func_local_vars;  // 函数局部变量
    std::unordered_map<std::string, std::vector<ParameterInfo>> func_params;  // 函数参数信息
    std::unordered_set<std::string> ref_params;  // 当前函数中的引用参数（var 参数）
    int temp_var_counter = 0;  // 临时变量计数器（用于副作用参数）
    std::vector<std::string> temp_declarations;
    std::unordered_set<std::string> temp_decl_names;
    
    void indent();
    std::string c_operator(BinaryOp op);
    std::string c_type(DataType t);
    std::string c_decl_type(DataType t, const RecordInfo* record_info = nullptr);
    std::string c_format_specifier(DataType t, bool for_scanf = false);
    void generate_record_definitions();
    void generate_forward_declarations();
    void collect_record_definition(const RecordInfo& record_info);
    std::optional<SymbolTypeInfo> lookup_type_info(const std::string& name) const;
    DataType get_identifier_type(const std::string& name);
    bool is_ref_param(const std::string& name);  // 检查是否是引用参数
    const std::vector<ParameterInfo>* lookup_callable_params(const std::string& name) const;
    std::string emitted_callable_name(const std::string& name) const;
    void remember_symbol_type(const std::string& name, DataType type, const ArrayInfo& array_info, const RecordInfo* record_info);
    void emit_decl_for_variable(const VariableDeclarationNode& n);
    void reset_temp_declarations();
    void emit_temp_declarations();
    std::string reserve_temp_name(const ResolvedType& type, bool is_pointer);
    void emit_call_argument_bindings(const std::string& callable_name, const std::vector<std::unique_ptr<ExpressionNode>>& arguments, int base);
    void emit_call_argument_list(const std::string& callable_name, const std::vector<std::unique_ptr<ExpressionNode>>& arguments);
    bool call_has_side_effects(const std::vector<std::unique_ptr<ExpressionNode>>& arguments) const;
    DataType get_expr_type(ExpressionNode* expr);  // 获取表达式类型
};

} // namespace pascal_s
