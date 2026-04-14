#pragma once

#include "ast.h"
#include <functional>
#include <optional>
#include <string>

namespace pascal_s {

struct SymbolTypeInfo {
    DataType type = DataType::TY_UNKNOWN;
    DataType return_type = DataType::TY_UNKNOWN;
    bool is_subprogram = false;
    ArrayInfo array_info;
    const RecordInfo* record_info = nullptr;

    DataType effective_type() const {
        return is_subprogram ? return_type : type;
    }
};

struct ResolvedType {
    DataType type = DataType::TY_UNKNOWN;
    ArrayInfo array_info;
    const RecordInfo* record_info = nullptr;
};

using SymbolLookup = std::function<std::optional<SymbolTypeInfo>(const std::string&)>;

const RecordField* find_record_field(const RecordInfo& record_info, const std::string& field_name);
ResolvedType resolve_expr_type(ExpressionNode* expr, const SymbolLookup& lookup);
const RecordInfo* resolve_record_info(ExpressionNode* expr, const SymbolLookup& lookup);
bool is_boolean_expr(ExpressionNode* expr, const SymbolLookup& lookup);

} // namespace pascal_s
