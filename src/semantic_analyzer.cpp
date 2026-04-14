#include "semantic_analyzer.h"
#include "error.h"

namespace pascal_s {

namespace {

const RecordField* find_record_field(const RecordInfo& record_info, const std::string& field_name) {
    for (const auto& field : record_info.fields) {
        if (field.name == field_name) {
            return &field;
        }
    }
    return nullptr;
}

const RecordInfo* resolve_record_info(const SymbolTable& sym_table, ExpressionNode* expr) {
    if (!expr) {
        return nullptr;
    }

    if (auto* ident = dynamic_cast<IdentifierNode*>(expr)) {
        auto symbol = sym_table.lookup(ident->name);
        return symbol ? &symbol->record_info : nullptr;
    }

    if (auto* access = dynamic_cast<ArrayAccessNode*>(expr)) {
        auto symbol = sym_table.lookup(access->array_name);
        if (symbol && symbol->array_info.element_type == DataType::TY_RECORD) {
            return &symbol->record_info;
        }
        return nullptr;
    }

    if (auto* record_access = dynamic_cast<RecordAccessNode*>(expr)) {
        const RecordInfo* base_info = resolve_record_info(sym_table, record_access->record_expr.get());
        if (!base_info) {
            return nullptr;
        }
        const RecordField* field = find_record_field(*base_info, record_access->field_name);
        if (field && field->type == DataType::TY_RECORD && field->record_info) {
            return field->record_info.get();
        }
    }

    return nullptr;
}

} // namespace

SemanticAnalyzer::SemanticAnalyzer() {
    sym_table.clear();
}

void SemanticAnalyzer::report_semantic_error(const ASTNode& node, const std::string& message) {
    ErrorHandler::instance().semantic_error(message, node.line, node.column);
}

std::shared_ptr<SymbolEntry> SemanticAnalyzer::lookup_symbol(const std::string& name) {
    return sym_table.lookup(name);
}

std::shared_ptr<const SymbolEntry> SemanticAnalyzer::lookup_symbol(const std::string& name) const {
    return sym_table.lookup(name);
}

bool SemanticAnalyzer::is_builtin_procedure(const std::string& name) const {
    return name == "read" || name == "readln" || name == "write" || name == "writeln" ||
           name == "break" || name == "continue";
}

void SemanticAnalyzer::visit(IntegerLiteralNode& /*n*/) {}
void SemanticAnalyzer::visit(RealLiteralNode& /*n*/) {}
void SemanticAnalyzer::visit(BooleanLiteralNode& /*n*/) {}
void SemanticAnalyzer::visit(CharLiteralNode& /*n*/) {}
void SemanticAnalyzer::visit(StringLiteralNode& /*n*/) {}

void SemanticAnalyzer::visit(IdentifierNode& n) {
    if (n.name == current_func_name) {
        return;
    }

    if (!lookup_symbol(n.name)) {
        report_semantic_error(n, "Undeclared identifier: " + n.name);
    }
}

void SemanticAnalyzer::visit(ArrayAccessNode& n) {
    auto symbol = lookup_symbol(n.array_name);
    if (!symbol) {
        report_semantic_error(n, "Undeclared array: " + n.array_name);
        return;
    }

    if (symbol->type != DataType::TY_ARRAY) {
        report_semantic_error(n, "'" + n.array_name + "' is not an array");
    }

    if (!symbol->array_info.dimensions.empty() &&
        n.indices.size() != symbol->array_info.dimensions.size()) {
        report_semantic_error(
            n,
            "Array '" + n.array_name + "' expects " +
                std::to_string(symbol->array_info.dimensions.size()) +
                " index(es), but got " + std::to_string(n.indices.size()));
    }

    for (auto& idx : n.indices) {
        if (!idx) {
            continue;
        }
        idx->accept(*this);
        DataType index_type = get_expr_type(idx.get());
        if (index_type != DataType::TY_INTEGER) {
            report_semantic_error(*idx, "Array index must be integer");
        }
    }
}

void SemanticAnalyzer::visit(RecordAccessNode& n) {
    if (n.record_expr) {
        n.record_expr->accept(*this);
    }

    const RecordInfo* record_info = resolve_record_info(sym_table, n.record_expr.get());
    if (!record_info) {
        report_semantic_error(n, "Field access requires a record value");
        return;
    }

    if (!find_record_field(*record_info, n.field_name)) {
        report_semantic_error(n, "Unknown record field: " + n.field_name);
    }
}

void SemanticAnalyzer::visit(BinaryExpressionNode& n) {
    if (n.left) {
        n.left->accept(*this);
    }
    if (n.right) {
        n.right->accept(*this);
    }

    DataType left_type = get_expr_type(n.left.get());
    DataType right_type = get_expr_type(n.right.get());

    switch (n.op) {
        case BinaryOp::OP_AND:
        case BinaryOp::OP_OR:
            if (left_type != DataType::TY_BOOLEAN || right_type != DataType::TY_BOOLEAN) {
                report_semantic_error(n, "Logical operators require boolean operands");
            }
            break;
        case BinaryOp::OP_DIV:
        case BinaryOp::OP_MOD:
            if (left_type != DataType::TY_INTEGER || right_type != DataType::TY_INTEGER) {
                report_semantic_error(n, "Operators 'div' and 'mod' require integer operands");
            }
            break;
        case BinaryOp::OP_ADD:
        case BinaryOp::OP_SUB:
        case BinaryOp::OP_MUL:
        case BinaryOp::OP_DIV_REAL:
            if ((left_type != DataType::TY_INTEGER && left_type != DataType::TY_REAL) ||
                (right_type != DataType::TY_INTEGER && right_type != DataType::TY_REAL)) {
                report_semantic_error(n, "Arithmetic operators require numeric operands");
            }
            break;
        case BinaryOp::OP_EQ:
        case BinaryOp::OP_NE:
        case BinaryOp::OP_LT:
        case BinaryOp::OP_LE:
        case BinaryOp::OP_GT:
        case BinaryOp::OP_GE:
            if (!TypeSystem::is_compatible(left_type, right_type) &&
                !TypeSystem::is_compatible(right_type, left_type)) {
                report_semantic_error(
                    n,
                    "Type mismatch in comparison: " + type_to_string(left_type) +
                        " vs " + type_to_string(right_type));
            }
            break;
    }
}

void SemanticAnalyzer::visit(UnaryExpressionNode& n) {
    if (n.operand) {
        n.operand->accept(*this);
    }

    DataType operand_type = get_expr_type(n.operand.get());
    if (n.op == UnaryOp::UOP_NOT &&
        operand_type != DataType::TY_BOOLEAN &&
        operand_type != DataType::TY_INTEGER) {
        report_semantic_error(n, "Operator 'not' requires a boolean or integer operand");
    }
    if (n.op == UnaryOp::UOP_NEGATE &&
        operand_type != DataType::TY_INTEGER &&
        operand_type != DataType::TY_REAL) {
        report_semantic_error(n, "Unary '-' requires a numeric operand");
    }
}

void SemanticAnalyzer::visit(FunctionCallNode& n) {
    auto symbol = lookup_symbol(n.func_name);
    if (!symbol) {
        report_semantic_error(n, "Undeclared function: " + n.func_name);
        return;
    }

    if (symbol->type != DataType::TY_FUNCTION) {
        report_semantic_error(n, "'" + n.func_name + "' is not a function");
    }

    if (n.arguments.size() != symbol->params.size()) {
        report_semantic_error(
            n,
            "Function '" + n.func_name + "' expects " +
                std::to_string(symbol->params.size()) + " argument(s), but got " +
                std::to_string(n.arguments.size()));
    }

    for (size_t i = 0; i < n.arguments.size(); ++i) {
        auto& arg = n.arguments[i];
        if (!arg) {
            continue;
        }
        arg->accept(*this);

        if (i >= symbol->params.size()) {
            continue;
        }

        const auto& param = symbol->params[i];
        DataType arg_type = get_expr_type(arg.get());
        if (!TypeSystem::is_compatible(arg_type, param.type)) {
            report_semantic_error(
                *arg,
                "Argument type mismatch for parameter '" + param.name + "'");
        }
        if (param.is_reference && !arg->is_lvalue()) {
            report_semantic_error(*arg, "Reference parameter requires an assignable argument");
        }
    }
}

void SemanticAnalyzer::visit(AssignmentNode& n) {
    if (n.target) {
        n.target->accept(*this);
    }
    if (n.value) {
        n.value->accept(*this);
    }

    if (n.target && !n.target->is_lvalue()) {
        report_semantic_error(*n.target, "Assignment target is not assignable");
        return;
    }

    if (auto* ident = dynamic_cast<IdentifierNode*>(n.target.get())) {
        if (ident->name != current_func_name) {
            auto symbol = lookup_symbol(ident->name);
            if (symbol && symbol->is_const) {
                report_semantic_error(*ident, "Cannot assign to constant '" + ident->name + "'");
            }
        }
    }

    DataType target_type = get_expr_type(n.target.get());
    DataType value_type = get_expr_type(n.value.get());
    if (!TypeSystem::is_compatible(value_type, target_type)) {
        report_semantic_error(
            n,
            "Type mismatch in assignment: cannot assign " +
                type_to_string(value_type) + " to " + type_to_string(target_type));
    }
}

void SemanticAnalyzer::visit(CompoundStatementNode& n) {
    for (auto& stmt : n.statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
}

void SemanticAnalyzer::visit(IfStatementNode& n) {
    if (n.condition) {
        n.condition->accept(*this);
        DataType cond_type = get_expr_type(n.condition.get());
        if (cond_type != DataType::TY_BOOLEAN && cond_type != DataType::TY_INTEGER) {
            report_semantic_error(*n.condition, "If condition must be boolean-compatible");
        }
    }
    if (n.then_branch) {
        n.then_branch->accept(*this);
    }
    if (n.else_branch) {
        n.else_branch->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileStatementNode& n) {
    if (n.condition) {
        n.condition->accept(*this);
        DataType cond_type = get_expr_type(n.condition.get());
        if (cond_type != DataType::TY_BOOLEAN && cond_type != DataType::TY_INTEGER) {
            report_semantic_error(*n.condition, "While condition must be boolean-compatible");
        }
    }
    if (n.body) {
        n.body->accept(*this);
    }
}

void SemanticAnalyzer::visit(ForStatementNode& n) {
    auto symbol = lookup_symbol(n.loop_var);
    if (!symbol) {
        ErrorHandler::instance().semantic_error(
            "Undeclared for-loop variable: " + n.loop_var, n.line, n.column);
        return;
    }

    if (symbol->type != DataType::TY_INTEGER) {
        ErrorHandler::instance().semantic_error(
            "For-loop variable must be integer: " + n.loop_var, n.line, n.column);
    }

    if (n.start) {
        n.start->accept(*this);
        if (get_expr_type(n.start.get()) != DataType::TY_INTEGER) {
            report_semantic_error(*n.start, "For-loop start expression must be integer");
        }
    }
    if (n.end) {
        n.end->accept(*this);
        if (get_expr_type(n.end.get()) != DataType::TY_INTEGER) {
            report_semantic_error(*n.end, "For-loop end expression must be integer");
        }
    }
    if (n.body) {
        n.body->accept(*this);
    }
}

void SemanticAnalyzer::visit(ProcedureCallNode& n) {
    if (is_builtin_procedure(n.proc_name)) {
        for (auto& arg : n.arguments) {
            if (arg) {
                arg->accept(*this);
            }
        }
        return;
    }

    auto symbol = lookup_symbol(n.proc_name);
    if (!symbol) {
        report_semantic_error(n, "Undeclared procedure: " + n.proc_name);
        return;
    }

    if (symbol->type != DataType::TY_PROCEDURE && symbol->type != DataType::TY_FUNCTION) {
        report_semantic_error(n, "'" + n.proc_name + "' is not callable");
    }

    if (n.arguments.size() != symbol->params.size()) {
        report_semantic_error(
            n,
            "Procedure '" + n.proc_name + "' expects " +
                std::to_string(symbol->params.size()) + " argument(s), but got " +
                std::to_string(n.arguments.size()));
    }

    for (size_t i = 0; i < n.arguments.size(); ++i) {
        auto& arg = n.arguments[i];
        if (!arg) {
            continue;
        }
        arg->accept(*this);

        if (i >= symbol->params.size()) {
            continue;
        }

        const auto& param = symbol->params[i];
        DataType arg_type = get_expr_type(arg.get());
        if (!TypeSystem::is_compatible(arg_type, param.type)) {
            report_semantic_error(
                *arg,
                "Argument type mismatch for parameter '" + param.name + "'");
        }
        if (param.is_reference && !arg->is_lvalue()) {
            report_semantic_error(*arg, "Reference parameter requires an assignable argument");
        }
    }
}

void SemanticAnalyzer::visit(WriteStatementNode& n) {
    if (n.value) {
        n.value->accept(*this);
    }
    for (auto& val : n.values) {
        if (val) {
            val->accept(*this);
        }
    }
}

void SemanticAnalyzer::visit(VariableDeclarationNode& n) {
    if (!sym_table.insert(n.var_name, n.type, n.is_const, false, n.array_info)) {
        report_semantic_error(n, "Duplicate identifier declaration: " + n.var_name);
        return;
    }

    auto symbol = lookup_symbol(n.var_name);
    if (symbol) {
        symbol->record_info = n.record_info;
    }

    if (n.init_value) {
        n.init_value->accept(*this);
        DataType init_type = get_expr_type(n.init_value.get());
        if (!TypeSystem::is_compatible(init_type, n.type)) {
            report_semantic_error(
                n,
                "Initializer type mismatch for '" + n.var_name + "'");
        }
    }
}

void SemanticAnalyzer::visit(FunctionDeclarationNode& n) {
    current_func_name = n.func_name;
    sym_table.enter_scope();

    for (const auto& param : n.parameters) {
        if (!sym_table.insert(param.name, param.type, false, param.is_reference, param.array_info)) {
            ErrorHandler::instance().semantic_error(
                "Duplicate parameter name: " + param.name, n.line, n.column);
            continue;
        }

        auto symbol = lookup_symbol(param.name);
        if (symbol) {
            symbol->record_info = param.record_info;
        }
    }

    for (const auto& var_decl : n.local_vars) {
        if (var_decl) {
            var_decl->accept(*this);
        }
    }

    if (n.body) {
        n.body->accept(*this);
    }

    sym_table.exit_scope();
    current_func_name.clear();
}

void SemanticAnalyzer::visit(ProgramNode& n) {
    for (const auto& decl : n.declarations) {
        auto* func = dynamic_cast<FunctionDeclarationNode*>(decl.get());
        if (!func) {
            continue;
        }
        if (sym_table.exists_in_current_scope(func->func_name)) {
            report_semantic_error(*func, "Duplicate subprogram declaration: " + func->func_name);
            continue;
        }
        sym_table.add_function(
            func->func_name,
            func->return_type,
            func->parameters,
            func->is_procedure);
    }

    for (auto& decl : n.declarations) {
        if (dynamic_cast<FunctionDeclarationNode*>(decl.get())) {
            continue;
        }
        if (decl) {
            decl->accept(*this);
        }
    }

    for (auto& decl : n.declarations) {
        if (auto* func = dynamic_cast<FunctionDeclarationNode*>(decl.get())) {
            func->accept(*this);
        }
    }

    if (n.main_body) {
        n.main_body->accept(*this);
    }
}

DataType SemanticAnalyzer::get_expr_type(ExpressionNode* expr) {
    if (!expr) {
        return DataType::TY_UNKNOWN;
    }
    if (dynamic_cast<IntegerLiteralNode*>(expr)) {
        return DataType::TY_INTEGER;
    }
    if (dynamic_cast<RealLiteralNode*>(expr)) {
        return DataType::TY_REAL;
    }
    if (dynamic_cast<BooleanLiteralNode*>(expr)) {
        return DataType::TY_BOOLEAN;
    }
    if (dynamic_cast<CharLiteralNode*>(expr)) {
        return DataType::TY_CHAR;
    }
    if (dynamic_cast<StringLiteralNode*>(expr)) {
        return DataType::TY_CHAR;
    }
    if (auto* ident = dynamic_cast<IdentifierNode*>(expr)) {
        return get_identifier_type(ident->name);
    }
    if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr)) {
        auto symbol = lookup_symbol(arr->array_name);
        if (!symbol) {
            return DataType::TY_UNKNOWN;
        }
        return symbol->array_info.element_type;
    }
    if (auto* record = dynamic_cast<RecordAccessNode*>(expr)) {
        const RecordInfo* base_info = resolve_record_info(sym_table, record->record_expr.get());
        if (!base_info) {
            return DataType::TY_UNKNOWN;
        }
        const RecordField* field = find_record_field(*base_info, record->field_name);
        return field ? field->type : DataType::TY_UNKNOWN;
    }
    if (auto* unary = dynamic_cast<UnaryExpressionNode*>(expr)) {
        if (unary->op == UnaryOp::UOP_NOT) {
            DataType operand_type = get_expr_type(unary->operand.get());
            if (operand_type == DataType::TY_INTEGER) {
                return DataType::TY_INTEGER;
            }
            return DataType::TY_BOOLEAN;
        }
        return get_expr_type(unary->operand.get());
    }
    if (auto* binary = dynamic_cast<BinaryExpressionNode*>(expr)) {
        return TypeSystem::infer_binary_result(
            binary->op,
            get_expr_type(binary->left.get()),
            get_expr_type(binary->right.get()));
    }
    if (auto* call = dynamic_cast<FunctionCallNode*>(expr)) {
        auto symbol = lookup_symbol(call->func_name);
        return symbol ? symbol->return_type : DataType::TY_UNKNOWN;
    }
    return DataType::TY_UNKNOWN;
}

DataType SemanticAnalyzer::get_identifier_type(const std::string& name) {
    if (name == current_func_name) {
        auto symbol = lookup_symbol(name);
        return symbol ? symbol->return_type : DataType::TY_UNKNOWN;
    }

    auto symbol = lookup_symbol(name);
    if (!symbol) {
        return DataType::TY_UNKNOWN;
    }
    if (symbol->is_subprogram()) {
        return symbol->return_type;
    }
    return symbol->type;
}

} // namespace pascal_s
