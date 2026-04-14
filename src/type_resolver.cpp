#include "type_resolver.h"
#include "symbol_table.h"

namespace pascal_s {

const RecordField* find_record_field(const RecordInfo& record_info, const std::string& field_name) {
    for (const auto& field : record_info.fields) {
        if (field.name == field_name) {
            return &field;
        }
    }
    return nullptr;
}

const RecordInfo* resolve_record_info(ExpressionNode* expr, const SymbolLookup& lookup) {
    if (!expr) {
        return nullptr;
    }

    if (auto* ident = dynamic_cast<IdentifierNode*>(expr)) {
        auto symbol = lookup(ident->name);
        return symbol ? symbol->record_info : nullptr;
    }

    if (auto* access = dynamic_cast<ArrayAccessNode*>(expr)) {
        auto symbol = lookup(access->array_name);
        if (symbol &&
            symbol->array_info.element_type == DataType::TY_RECORD &&
            symbol->record_info) {
            return symbol->record_info;
        }
        return nullptr;
    }

    if (auto* record_access = dynamic_cast<RecordAccessNode*>(expr)) {
        const RecordInfo* base_info = resolve_record_info(record_access->record_expr.get(), lookup);
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

ResolvedType resolve_expr_type(ExpressionNode* expr, const SymbolLookup& lookup) {
    if (!expr) {
        return {};
    }

    if (dynamic_cast<IntegerLiteralNode*>(expr)) {
        return {DataType::TY_INTEGER, {}, nullptr};
    }
    if (dynamic_cast<RealLiteralNode*>(expr)) {
        return {DataType::TY_REAL, {}, nullptr};
    }
    if (dynamic_cast<BooleanLiteralNode*>(expr)) {
        return {DataType::TY_BOOLEAN, {}, nullptr};
    }
    if (dynamic_cast<CharLiteralNode*>(expr)) {
        return {DataType::TY_CHAR, {}, nullptr};
    }
    if (dynamic_cast<StringLiteralNode*>(expr)) {
        return {DataType::TY_CHAR, {}, nullptr};
    }

    if (auto* ident = dynamic_cast<IdentifierNode*>(expr)) {
        auto symbol = lookup(ident->name);
        if (!symbol) {
            return {};
        }
        return {symbol->effective_type(), symbol->array_info, symbol->record_info};
    }

    if (auto* access = dynamic_cast<ArrayAccessNode*>(expr)) {
        auto symbol = lookup(access->array_name);
        if (!symbol) {
            return {};
        }
        if (symbol->array_info.element_type != DataType::TY_UNKNOWN) {
            const RecordInfo* record_info =
                symbol->array_info.element_type == DataType::TY_RECORD ? symbol->record_info : nullptr;
            return {symbol->array_info.element_type, {}, record_info};
        }
        return {symbol->effective_type(), symbol->array_info, symbol->record_info};
    }

    if (auto* record = dynamic_cast<RecordAccessNode*>(expr)) {
        const RecordInfo* base_info = resolve_record_info(record->record_expr.get(), lookup);
        if (!base_info) {
            return {};
        }
        const RecordField* field = find_record_field(*base_info, record->field_name);
        if (!field) {
            return {};
        }
        return {field->type, field->array_info, field->record_info.get()};
    }

    if (auto* unary = dynamic_cast<UnaryExpressionNode*>(expr)) {
        if (unary->op == UnaryOp::UOP_NOT) {
            ResolvedType operand = resolve_expr_type(unary->operand.get(), lookup);
            if (operand.type == DataType::TY_INTEGER) {
                return {DataType::TY_INTEGER, {}, nullptr};
            }
            return {DataType::TY_BOOLEAN, {}, nullptr};
        }
        return resolve_expr_type(unary->operand.get(), lookup);
    }

    if (auto* binary = dynamic_cast<BinaryExpressionNode*>(expr)) {
        ResolvedType left = resolve_expr_type(binary->left.get(), lookup);
        ResolvedType right = resolve_expr_type(binary->right.get(), lookup);
        return {TypeSystem::infer_binary_result(binary->op, left.type, right.type), {}, nullptr};
    }

    if (auto* call = dynamic_cast<FunctionCallNode*>(expr)) {
        auto symbol = lookup(call->func_name);
        return symbol ? ResolvedType{symbol->return_type, {}, nullptr} : ResolvedType{};
    }

    return {};
}

bool is_boolean_expr(ExpressionNode* expr, const SymbolLookup& lookup) {
    return resolve_expr_type(expr, lookup).type == DataType::TY_BOOLEAN;
}

} // namespace pascal_s
