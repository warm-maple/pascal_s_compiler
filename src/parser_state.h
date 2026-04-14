#pragma once

#include "ast.h"
#include <string>
#include <vector>

namespace pascal_s {

struct ParserState {
    std::vector<ParameterInfo> func_params;
    DataType last_func_return_type = DataType::TY_UNKNOWN;
    bool last_func_is_proc = false;
    std::vector<StatementNode*> stmt_list;
    std::vector<std::vector<StatementNode*>> stmt_list_stack;
    std::vector<std::vector<ExpressionNode*>> arg_list_stack;
    StatementNode* stmt_result = nullptr;
    std::string last_func_name;
    std::vector<FunctionDeclarationNode*> pending_func_decls;
    std::vector<VariableDeclarationNode*> pending_var_decls;
    std::vector<std::vector<VariableDeclarationNode*>> pending_local_var_decls;
    ArrayInfo last_array_info;
    std::vector<ArrayInfo> array_info_stack;
    RecordInfo last_record_info;
    std::vector<RecordInfo> record_info_stack;
    int next_record_struct_id = 0;

    void reset();
    void reset_type_side_data();
};

ParserState& parser_state();
void reset_parser_state();

} // namespace pascal_s
