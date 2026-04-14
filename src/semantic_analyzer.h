#pragma once
#include "ast.h"
#include "symbol_table.h"
#include "type_resolver.h"
#include <string>
#include <unordered_map>

namespace pascal_s {

class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer();
    ~SemanticAnalyzer() = default;

    // Symbol Table owned by Semantic Analyzer
    SymbolTable sym_table;
    const SymbolTable& symbol_table() const { return sym_table; }

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
    std::string current_func_name;
    void report_semantic_error(const ASTNode& node, const std::string& message);
    std::shared_ptr<SymbolEntry> lookup_symbol(const std::string& name);
    std::shared_ptr<const SymbolEntry> lookup_symbol(const std::string& name) const;
    bool is_builtin_procedure(const std::string& name) const;
    std::optional<SymbolTypeInfo> lookup_type_info(const std::string& name) const;

    DataType get_expr_type(ExpressionNode* expr);
};

} // namespace pascal_s
