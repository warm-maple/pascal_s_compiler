#include "parser_state.h"

namespace pascal_s {

namespace {

ParserState g_parser_state;

}

void ParserState::reset() {
    func_params.clear();
    last_func_return_type = DataType::TY_UNKNOWN;
    last_func_is_proc = false;
    stmt_list.clear();
    stmt_list_stack.clear();
    arg_list_stack.clear();
    stmt_result = nullptr;
    last_func_name.clear();
    pending_func_decls.clear();
    pending_var_decls.clear();
    pending_local_var_decls.clear();
    last_array_info = ArrayInfo{};
    array_info_stack.clear();
    last_record_info = RecordInfo{};
    record_info_stack.clear();
    next_record_struct_id = 0;
}

void ParserState::reset_type_side_data() {
    last_array_info = ArrayInfo{};
    last_record_info = RecordInfo{};
}

ParserState& parser_state() {
    return g_parser_state;
}

void reset_parser_state() {
    g_parser_state.reset();
}

} // namespace pascal_s
