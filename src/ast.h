#pragma once
#include <string>
#include <vector>
#include <memory>
#include <sstream>

namespace pascal_s {

// 前向声明
class ASTVisitor;
class ExpressionNode;
class StatementNode;
class DeclarationNode;

// 数据类型枚举
enum class DataType {
    TY_INTEGER,
    TY_REAL,
    TY_BOOLEAN,
    TY_CHAR,
    TY_ARRAY,
    TY_FUNCTION,
    TY_PROCEDURE,
    TY_VOID,
    TY_UNKNOWN
};

// 数组类型信息
struct ArrayInfo {
    int lower_bound = 0;
    int upper_bound = 0;
    DataType element_type = DataType::TY_INTEGER;
};

// 参数信息
struct ParameterInfo {
    std::string name;
    DataType type = DataType::TY_INTEGER;
    bool is_reference = false;  // var 参数
    ArrayInfo array_info;
};

// AST 节点基类
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string node_type() const = 0;
    virtual void accept(ASTVisitor& visitor) = 0;
    
    int line = 0;
    int column = 0;
};

// ============ 表达式节点 ============

class ExpressionNode : public ASTNode {
public:
    virtual bool is_lvalue() const { return false; }
    virtual DataType get_type() const { return DataType::TY_UNKNOWN; }
};

// 字面量
class IntegerLiteralNode : public ExpressionNode {
public:
    int value;
    IntegerLiteralNode(int v = 0) : value(v) {}
    std::string node_type() const override { return "IntegerLiteral"; }
    void accept(ASTVisitor& visitor) override;
    DataType get_type() const override { return DataType::TY_INTEGER; }
};

class RealLiteralNode : public ExpressionNode {
public:
    double value;
    RealLiteralNode(double v = 0.0) : value(v) {}
    std::string node_type() const override { return "RealLiteral"; }
    void accept(ASTVisitor& visitor) override;
    DataType get_type() const override { return DataType::TY_REAL; }
};

class CharLiteralNode : public ExpressionNode {
public:
    char value;
    CharLiteralNode(char v = '\0') : value(v) {}
    std::string node_type() const override { return "CharLiteral"; }
    void accept(ASTVisitor& visitor) override;
    DataType get_type() const override { return DataType::TY_CHAR; }
};

class StringLiteralNode : public ExpressionNode {
public:
    std::string value;
    StringLiteralNode(const std::string& v = "") : value(v) {}
    std::string node_type() const override { return "StringLiteral"; }
    void accept(ASTVisitor& visitor) override;
};

// 标识符 (变量/函数)
class IdentifierNode : public ExpressionNode {
public:
    std::string name;
    IdentifierNode(const std::string& n = "") : name(n) {}
    std::string node_type() const override { return "Identifier"; }
    void accept(ASTVisitor& visitor) override;
    bool is_lvalue() const override { return true; }
};

// 数组访问
class ArrayAccessNode : public ExpressionNode {
public:
    std::string array_name;
    std::vector<std::unique_ptr<ExpressionNode>> indices;
    ArrayAccessNode(const std::string& name = "")
        : array_name(name) {}
    ArrayAccessNode(const std::string& name, std::unique_ptr<ExpressionNode> idx)
        : array_name(name) { indices.push_back(std::move(idx)); }
    void add_index(std::unique_ptr<ExpressionNode> idx) {
        indices.push_back(std::move(idx));
    }
    std::string node_type() const override { return "ArrayAccess"; }
    void accept(ASTVisitor& visitor) override;
    bool is_lvalue() const override { return true; }
};

// 二元运算符
enum class BinaryOp { 
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_AND, OP_OR,
    OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE 
};

class BinaryExpressionNode : public ExpressionNode {
public:
    BinaryOp op;
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;
    
    BinaryExpressionNode(BinaryOp o = BinaryOp::OP_ADD,
                         std::unique_ptr<ExpressionNode> l = nullptr,
                         std::unique_ptr<ExpressionNode> r = nullptr)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    
    std::string node_type() const override { return "BinaryExpression"; }
    void accept(ASTVisitor& visitor) override;
};

// 一元运算符
enum class UnaryOp { UOP_NOT, UOP_NEGATE };

class UnaryExpressionNode : public ExpressionNode {
public:
    UnaryOp op;
    std::unique_ptr<ExpressionNode> operand;
    
    UnaryExpressionNode(UnaryOp o = UnaryOp::UOP_NOT, std::unique_ptr<ExpressionNode> e = nullptr)
        : op(o), operand(std::move(e)) {}
    
    std::string node_type() const override { return "UnaryExpression"; }
    void accept(ASTVisitor& visitor) override;
};

// 函数调用
class FunctionCallNode : public ExpressionNode {
public:
    std::string func_name;
    std::vector<std::unique_ptr<ExpressionNode>> arguments;
    
    FunctionCallNode(const std::string& name = "") : func_name(name) {}
    
    void add_argument(std::unique_ptr<ExpressionNode> arg) {
        arguments.push_back(std::move(arg));
    }
    
    void add_argument(ExpressionNode* arg) {
        if (arg) arguments.push_back(std::unique_ptr<ExpressionNode>(arg));
    }
    
    std::string node_type() const override { return "FunctionCall"; }
    void accept(ASTVisitor& visitor) override;
};

// ============ 语句节点 ============

class StatementNode : public ASTNode {};

// 赋值语句
class AssignmentNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> target;
    std::unique_ptr<ExpressionNode> value;
    
    AssignmentNode(std::unique_ptr<ExpressionNode> t = nullptr,
                   std::unique_ptr<ExpressionNode> v = nullptr)
        : target(std::move(t)), value(std::move(v)) {}
    
    AssignmentNode(ExpressionNode* t, ExpressionNode* v)
        : target(t ? std::unique_ptr<ExpressionNode>(t) : nullptr),
          value(v ? std::unique_ptr<ExpressionNode>(v) : nullptr) {}
    
    std::string node_type() const override { return "Assignment"; }
    void accept(ASTVisitor& visitor) override;
};

// 复合语句 (begin...end)
class CompoundStatementNode : public StatementNode {
public:
    std::vector<std::unique_ptr<StatementNode>> statements;
    
    void add_statement(std::unique_ptr<StatementNode> stmt) {
        statements.push_back(std::move(stmt));
    }
    
    std::string node_type() const override { return "CompoundStatement"; }
    void accept(ASTVisitor& visitor) override;
};

// if 语句
class IfStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<StatementNode> then_branch;
    std::unique_ptr<StatementNode> else_branch;
    
    IfStatementNode(std::unique_ptr<ExpressionNode> cond = nullptr,
                    std::unique_ptr<StatementNode> then_stmt = nullptr)
        : condition(std::move(cond)), then_branch(std::move(then_stmt)) {}
    
    IfStatementNode(ExpressionNode* cond, StatementNode* then_stmt)
        : condition(cond ? std::unique_ptr<ExpressionNode>(cond) : nullptr),
          then_branch(then_stmt ? std::unique_ptr<StatementNode>(then_stmt) : nullptr) {}
    
    void set_else_branch(std::unique_ptr<StatementNode> else_stmt) {
        else_branch = std::move(else_stmt);
    }
    
    std::string node_type() const override { return "IfStatement"; }
    void accept(ASTVisitor& visitor) override;
};

// while 循环
class WhileStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<StatementNode> body;
    
    WhileStatementNode(std::unique_ptr<ExpressionNode> cond = nullptr,
                       std::unique_ptr<StatementNode> b = nullptr)
        : condition(std::move(cond)), body(std::move(b)) {}
    
    WhileStatementNode(ExpressionNode* cond, StatementNode* b)
        : condition(cond ? std::unique_ptr<ExpressionNode>(cond) : nullptr),
          body(b ? std::unique_ptr<StatementNode>(b) : nullptr) {}
    
    std::string node_type() const override { return "WhileStatement"; }
    void accept(ASTVisitor& visitor) override;
};

// for 循环
class ForStatementNode : public StatementNode {
public:
    std::string loop_var;
    std::unique_ptr<ExpressionNode> start;
    std::unique_ptr<ExpressionNode> end;
    std::unique_ptr<StatementNode> body;
    bool is_downto;
    
    ForStatementNode(const std::string& var = "",
                     std::unique_ptr<ExpressionNode> s = nullptr,
                     std::unique_ptr<ExpressionNode> e = nullptr,
                     std::unique_ptr<StatementNode> b = nullptr,
                     bool down = false)
        : loop_var(var), start(std::move(s)), end(std::move(e)),
          body(std::move(b)), is_downto(down) {}
    
    ForStatementNode(const std::string& var, ExpressionNode* s, ExpressionNode* e, StatementNode* b, bool down = false)
        : loop_var(var), start(s ? std::unique_ptr<ExpressionNode>(s) : nullptr),
          end(e ? std::unique_ptr<ExpressionNode>(e) : nullptr),
          body(b ? std::unique_ptr<StatementNode>(b) : nullptr), is_downto(down) {}
    
    std::string node_type() const override { return "ForStatement"; }
    void accept(ASTVisitor& visitor) override;
};

// 过程调用语句
class ProcedureCallNode : public StatementNode {
public:
    std::string proc_name;
    std::vector<std::unique_ptr<ExpressionNode>> arguments;
    
    ProcedureCallNode(const std::string& name = "") : proc_name(name) {}
    
    void add_argument(std::unique_ptr<ExpressionNode> arg) {
        arguments.push_back(std::move(arg));
    }
    
    void add_argument(ExpressionNode* arg) {
        if (arg) arguments.push_back(std::unique_ptr<ExpressionNode>(arg));
    }
    
    std::string node_type() const override { return "ProcedureCall"; }
    void accept(ASTVisitor& visitor) override;
};

// 写语句
class WriteStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> value;  // 保留向后兼容
    std::vector<std::unique_ptr<ExpressionNode>> values;  // 支持多个值
    
    WriteStatementNode(std::unique_ptr<ExpressionNode> v = nullptr)
        : value(std::move(v)) {}
    WriteStatementNode(ExpressionNode* v)
        : value(v ? std::unique_ptr<ExpressionNode>(v) : nullptr) {}
    
    void add_value(std::unique_ptr<ExpressionNode> v) {
        values.push_back(std::move(v));
    }
    
    std::string node_type() const override { return "WriteStatement"; }
    void accept(ASTVisitor& visitor) override;
};

// ============ 声明节点 ============

class DeclarationNode : public ASTNode {};

// 变量声明
class VariableDeclarationNode : public DeclarationNode {
public:
    std::string var_name;
    DataType type = DataType::TY_INTEGER;
    bool is_array = false;
    ArrayInfo array_info;
    bool is_const = false;
    std::unique_ptr<ExpressionNode> init_value;
    
    VariableDeclarationNode(const std::string& name = "", DataType t = DataType::TY_INTEGER)
        : var_name(name), type(t) {}
    
    std::string node_type() const override { return "VariableDeclaration"; }
    void accept(ASTVisitor& visitor) override;
};

// 函数/过程声明
class FunctionDeclarationNode : public DeclarationNode {
public:
    std::string func_name;
    DataType return_type = DataType::TY_VOID;
    std::vector<ParameterInfo> parameters;
    std::unique_ptr<CompoundStatementNode> body;
    bool is_procedure = false;
    
    FunctionDeclarationNode(const std::string& name = "", bool is_proc = false)
        : func_name(name), is_procedure(is_proc) {}
    
    std::string node_type() const override { return "FunctionDeclaration"; }
    void accept(ASTVisitor& visitor) override;
};

// ============ 程序根节点 ============

class ProgramNode : public ASTNode {
public:
    std::string program_name;
    std::vector<std::unique_ptr<DeclarationNode>> declarations;
    std::unique_ptr<CompoundStatementNode> main_body;
    
    ProgramNode(const std::string& name = "") : program_name(name) {}
    
    void add_declaration(std::unique_ptr<DeclarationNode> decl) {
        declarations.push_back(std::move(decl));
    }
    
    std::string node_type() const override { return "Program"; }
    void accept(ASTVisitor& visitor) override;
};

// ============ 访问者模式 ============

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(IntegerLiteralNode& n) = 0;
    virtual void visit(RealLiteralNode& n) = 0;
    virtual void visit(CharLiteralNode& n) = 0;
    virtual void visit(StringLiteralNode& n) = 0;
    virtual void visit(IdentifierNode& n) = 0;
    virtual void visit(ArrayAccessNode& n) = 0;
    virtual void visit(BinaryExpressionNode& n) = 0;
    virtual void visit(UnaryExpressionNode& n) = 0;
    virtual void visit(FunctionCallNode& n) = 0;
    virtual void visit(AssignmentNode& n) = 0;
    virtual void visit(CompoundStatementNode& n) = 0;
    virtual void visit(IfStatementNode& n) = 0;
    virtual void visit(WhileStatementNode& n) = 0;
    virtual void visit(ForStatementNode& n) = 0;
    virtual void visit(ProcedureCallNode& n) = 0;
    virtual void visit(WriteStatementNode& n) = 0;
    virtual void visit(VariableDeclarationNode& n) = 0;
    virtual void visit(FunctionDeclarationNode& n) = 0;
    virtual void visit(ProgramNode& n) = 0;
};

// 工具函数：类型转字符串
inline std::string type_to_string(DataType t) {
    switch (t) {
        case DataType::TY_INTEGER: return "integer";
        case DataType::TY_REAL: return "real";
        case DataType::TY_BOOLEAN: return "boolean";
        case DataType::TY_CHAR: return "char";
        case DataType::TY_ARRAY: return "array";
        case DataType::TY_FUNCTION: return "function";
        case DataType::TY_PROCEDURE: return "procedure";
        case DataType::TY_VOID: return "void";
        default: return "unknown";
    }
}

// 工具函数：关系运算符转换
inline BinaryOp relop_to_binop(const std::string& op) {
    if (op == "=") return BinaryOp::OP_EQ;
    if (op == "<>") return BinaryOp::OP_NE;
    if (op == "<") return BinaryOp::OP_LT;
    if (op == "<=") return BinaryOp::OP_LE;
    if (op == ">") return BinaryOp::OP_GT;
    if (op == ">=") return BinaryOp::OP_GE;
    return BinaryOp::OP_EQ;
}

// 工具函数：加法运算符转换
inline BinaryOp addop_to_binop(const std::string& op) {
    if (op == "+") return BinaryOp::OP_ADD;
    if (op == "-") return BinaryOp::OP_SUB;
    return BinaryOp::OP_ADD;
}

// 工具函数：乘法运算符转换
inline BinaryOp mulop_to_binop(const std::string& op) {
    if (op == "*") return BinaryOp::OP_MUL;
    if (op == "/") return BinaryOp::OP_DIV;
    return BinaryOp::OP_MUL;
}

} // namespace pascal_s
