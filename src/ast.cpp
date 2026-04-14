#include "ast.h"

namespace pascal_s {

// 表达式节点 accept 实现
void IntegerLiteralNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void RealLiteralNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BooleanLiteralNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CharLiteralNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void StringLiteralNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IdentifierNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ArrayAccessNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void RecordAccessNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BinaryExpressionNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void UnaryExpressionNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void FunctionCallNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// 语句节点 accept 实现
void AssignmentNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CompoundStatementNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IfStatementNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void WhileStatementNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ForStatementNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ProcedureCallNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void WriteStatementNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// 声明节点 accept 实现
void VariableDeclarationNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void FunctionDeclarationNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// 程序节点 accept 实现
void ProgramNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

} // namespace pascal_s
