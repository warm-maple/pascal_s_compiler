#include "codegen.h"

namespace pascal_s {

// Utilities and local functions can be declared here or copied verbatim over without `inline` keywords.

// CodeGenerator::c_operator
std::string CodeGenerator::c_operator(BinaryOp op) {
    switch (op) {
        case BinaryOp::OP_ADD: return "+";
        case BinaryOp::OP_SUB: return "-";
        case BinaryOp::OP_MUL: return "*";
        case BinaryOp::OP_DIV: return "/";
        case BinaryOp::OP_DIV_REAL: return "/";
        case BinaryOp::OP_MOD: return "%";
        case BinaryOp::OP_AND: return "&&";
        case BinaryOp::OP_OR: return "||";
        case BinaryOp::OP_EQ: return "==";
        case BinaryOp::OP_NE: return "!=";
        case BinaryOp::OP_LT: return "<";
        case BinaryOp::OP_LE: return "<=";
        case BinaryOp::OP_GT: return ">";
        case BinaryOp::OP_GE: return ">=";
        default: return "?";
    }
}

std::string CodeGenerator::c_type(DataType t) {
    // Change TY_REAL mapping from double to float
    if (t == DataType::TY_REAL) {
        return "float";
    }
    return TypeSystem::to_c_type(t);
}

std::string CodeGenerator::c_decl_type(DataType t, const RecordInfo* record_info) {
    if (t == DataType::TY_RECORD && record_info && !record_info->struct_name.empty()) {
        return "struct " + record_info->struct_name;
    }
    return c_type(t);
}

std::string CodeGenerator::c_format_specifier(DataType t, bool for_scanf) {
    switch (t) {
        case DataType::TY_INTEGER: return "%d";
        case DataType::TY_REAL: return for_scanf ? "%f" : "%f";
        case DataType::TY_BOOLEAN: return "%d";
        case DataType::TY_CHAR: return "%c";
        default: return "%d";
    }
}

void CodeGenerator::indent() {
    for (int i = 0; i < indent_level; i++) {
        output << "    ";
    }
}

void CodeGenerator::collect_record_definition(const RecordInfo& record_info) {
    if (record_info.struct_name.empty()) {
        return;
    }

    for (const auto& existing : record_definitions) {
        if (existing.find("struct " + record_info.struct_name + " {") != std::string::npos) {
            return;
        }
    }

    for (const auto& field : record_info.fields) {
        if (field.record_info) {
            collect_record_definition(*field.record_info);
        }
    }

    std::ostringstream def;
    def << "struct " << record_info.struct_name << " {\n";
    for (const auto& field : record_info.fields) {
        def << "    ";
        if (field.type == DataType::TY_ARRAY) {
            const RecordInfo* nested_info =
                (field.array_info.element_type == DataType::TY_RECORD && field.record_info)
                    ? field.record_info.get()
                    : nullptr;
            def << c_decl_type(field.array_info.element_type, nested_info) << " " << field.name;
            for (const auto& dim : field.array_info.dimensions) {
                def << "[" << dim.size() << "]";
            }
        } else {
            def << c_decl_type(field.type, field.record_info.get()) << " " << field.name;
        }
        def << ";\n";
    }
    def << "}";
    record_definitions.push_back(def.str());
}

void CodeGenerator::generate_record_definitions() {
    for (const auto& def : record_definitions) {
        output << def << ";\n\n";
    }
}

void CodeGenerator::generate_forward_declarations() {
    for (const auto& decl : forward_declarations) {
        output << decl << ";\n";
    }
    if (!forward_declarations.empty()) {
        output << "\n";
    }
}

std::optional<SymbolTypeInfo> CodeGenerator::lookup_type_info(const std::string& name) const {
    auto it = var_types.find(name);
    if (it != var_types.end()) {
        return it->second;
    }

    auto sym = g_symbol_table.lookup(name);
    if (!sym) {
        return std::nullopt;
    }

    SymbolTypeInfo info;
    info.type = sym->type;
    info.return_type = sym->return_type;
    info.is_subprogram = sym->is_subprogram();
    info.array_info = sym->array_info;
    info.record_info = &sym->record_info;
    return info;
}

const std::vector<ParameterInfo>* CodeGenerator::lookup_callable_params(const std::string& name) const {
    auto it = func_params.find(name);
    return it != func_params.end() ? &it->second : nullptr;
}

std::string CodeGenerator::emitted_callable_name(const std::string& name) const {
    return name == "main" ? "_pascal_main" : name;
}

void CodeGenerator::remember_symbol_type(
    const std::string& name,
    DataType type,
    const ArrayInfo& array_info,
    const RecordInfo* record_info) {
    SymbolTypeInfo info;
    info.type = type;
    info.array_info = array_info;
    info.record_info = record_info;
    var_types[name] = info;
}

void CodeGenerator::reset_temp_declarations() {
    temp_declarations.clear();
    temp_decl_names.clear();
}

void CodeGenerator::emit_temp_declarations() {
    for (const auto& decl : temp_declarations) {
        indent();
        output << decl << ";\n";
    }
}

std::string CodeGenerator::reserve_temp_name(const ResolvedType& type, bool is_pointer) {
    std::string temp_name = "_tv" + std::to_string(temp_var_counter++);
    if (!temp_decl_names.insert(temp_name).second) {
        return temp_name;
    }

    std::ostringstream decl;
    decl << c_decl_type(type.type, type.record_info);
    if (is_pointer) {
        decl << "*";
    }
    decl << " " << temp_name;
    temp_declarations.push_back(decl.str());
    return temp_name;
}

void CodeGenerator::emit_decl_for_variable(const VariableDeclarationNode& n) {
    indent();
    if (n.type == DataType::TY_ARRAY || n.is_array) {
        std::string elem_type = "int";
        if (n.array_info.element_type != DataType::TY_UNKNOWN) {
            const RecordInfo* record_info =
                n.array_info.element_type == DataType::TY_RECORD ? &n.record_info : nullptr;
            elem_type = c_decl_type(n.array_info.element_type, record_info);
        }

        output << elem_type << " " << n.var_name;

        if (!n.array_info.dimensions.empty()) {
            for (const auto& dim : n.array_info.dimensions) {
                output << "[" << dim.size() << "]";
            }
        } else if (n.array_info.upper_bound >= n.array_info.lower_bound) {
            int size = n.array_info.upper_bound - n.array_info.lower_bound + 1;
            output << "[" << size << "]";
        } else {
            output << "[1]";
        }
        output << ";\n";
        return;
    }

    if (n.type == DataType::TY_RECORD) {
        output << c_decl_type(n.type, &n.record_info) << " " << n.var_name << ";\n";
        return;
    }

    output << c_type(n.type) << " " << n.var_name << ";\n";
}

std::string CodeGenerator::generate(ProgramNode* program) {
    output.str("");
    output.clear();
    record_definitions.clear();
    forward_declarations.clear();
    indent_level = 0;
    var_types.clear();
    
    // C 头文件
    output << "#include <stdio.h>\n";
    output << "#include <stdlib.h>\n";
    output << "#include <math.h>\n";
    output << "#include <stdbool.h>\n\n";

    for (const auto& decl : program->declarations) {
        if (auto* var = dynamic_cast<VariableDeclarationNode*>(decl.get())) {
            if (var->type == DataType::TY_RECORD ||
                var->array_info.element_type == DataType::TY_RECORD) {
                collect_record_definition(var->record_info);
            }
        } else if (auto* func = dynamic_cast<FunctionDeclarationNode*>(decl.get())) {
            for (const auto& param : func->parameters) {
                if (param.type == DataType::TY_RECORD ||
                    param.array_info.element_type == DataType::TY_RECORD) {
                    collect_record_definition(param.record_info);
                }
            }
            for (const auto& local : func->local_vars) {
                if (local->type == DataType::TY_RECORD ||
                    local->array_info.element_type == DataType::TY_RECORD) {
                    collect_record_definition(local->record_info);
                }
            }
        }
    }

    generate_record_definitions();
    
    std::vector<FunctionDeclarationNode*> funcs;
    for (const auto& decl : program->declarations) {
        if (auto* func = dynamic_cast<FunctionDeclarationNode*>(decl.get())) {
            std::string fname = func->func_name;
            if (fname == "main") fname = "_pascal_main";
            std::ostringstream ss;
            ss << c_type(func->return_type) << " " << fname << "(";
            bool first = true;
            for (const auto& param : func->parameters) {
                if (!first) ss << ", ";
                first = false;
                if (param.is_reference) {
                    ss << c_decl_type(param.type, &param.record_info) << "* " << param.name;
                } else {
                    ss << c_decl_type(param.type, &param.record_info) << " " << param.name;
                }
            }
            ss << ")";
            forward_declarations.push_back(ss.str());
            funcs.push_back(func);
        }
    }
    
    func_local_vars.clear();
    for (const auto& decl : program->declarations) {
        if (auto* func = dynamic_cast<FunctionDeclarationNode*>(decl.get())) {
            for (const auto& var : func->local_vars) {
                func_local_vars[func->func_name].push_back(var.get());
            }
        }
    }
    
    generate_forward_declarations();
    
    std::unordered_set<std::string> declared_globals;
    for (const auto& decl : program->declarations) {
        if (auto* var = dynamic_cast<VariableDeclarationNode*>(decl.get())) {
            if (declared_globals.find(var->var_name) == declared_globals.end()) {
                decl->accept(*this);
                declared_globals.insert(var->var_name);
            }
        }
    }
    if (!declared_globals.empty()) {
        output << "\n";
    }
    
    for (const auto& decl : program->declarations) {
        if (dynamic_cast<FunctionDeclarationNode*>(decl.get())) {
            decl->accept(*this);
        }
    }
    
    ref_params.clear();
    current_func_name = "";
    
    output << "int main() {\n";
    indent_level++;

    reset_temp_declarations();
    std::ostringstream main_body;
    auto* old_buf = output.rdbuf();
    static_cast<std::ostream&>(output).rdbuf(main_body.rdbuf());

    if (program->main_body) {
        for (const auto& stmt : program->main_body->statements) {
            if (stmt) stmt->accept(*this);
        }
    }
    
    indent();
    output << "return 0;\n";

    static_cast<std::ostream&>(output).rdbuf(old_buf);
    emit_temp_declarations();
    output << main_body.str();

    indent_level--;
    output << "}\n";
    
    return output.str();
}

void CodeGenerator::visit(IntegerLiteralNode& n) {
    output << n.value;
}

void CodeGenerator::visit(RealLiteralNode& n) {
    std::ostringstream tmp;
    tmp.precision(15);
    tmp << n.value;
    std::string s = tmp.str();
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos && s.find('E') == std::string::npos) {
        s += ".0";
    }
    output << s;
}

void CodeGenerator::visit(BooleanLiteralNode& n) {
    output << (n.value ? "1" : "0");
}

void CodeGenerator::visit(CharLiteralNode& n) {
    output << "'" << n.value << "'";
}

void CodeGenerator::visit(StringLiteralNode& n) {
    output << "\"" << n.value << "\"";
}

void CodeGenerator::visit(IdentifierNode& n) {
    auto sym_entry = g_symbol_table.lookup(n.name);
    bool is_function = (sym_entry && sym_entry->is_subprogram());
    
    if (is_ref_param(n.name)) {
        output << "*";
    }
    std::string emit_name = (is_function && n.name == "main") ? "_pascal_main" : n.name;
    output << emit_name;
    
    if (is_function) {
        output << "()";
    }
}

void CodeGenerator::visit(ArrayAccessNode& n) {
    output << n.array_name;
    auto sym = g_symbol_table.lookup(n.array_name);
    for (size_t i = 0; i < n.indices.size(); i++) {
        output << "[";
        if (sym && i < sym->array_info.dimensions.size()) {
            int lower_bound = sym->array_info.dimensions[i].lower_bound;
            if (lower_bound != 0) {
                output << "(";
                n.indices[i]->accept(*this);
                output << ") - (" << lower_bound << ")";
            } else {
                n.indices[i]->accept(*this);
            }
        } else {
            n.indices[i]->accept(*this);
        }
        output << "]";
    }
}

void CodeGenerator::visit(RecordAccessNode& n) {
    if (n.record_expr) {
        n.record_expr->accept(*this);
        output << ".";
    }
    output << n.field_name;
}

static int get_precedence(BinaryOp op) {
    switch (op) {
        case BinaryOp::OP_MUL: case BinaryOp::OP_DIV: case BinaryOp::OP_DIV_REAL: case BinaryOp::OP_MOD: case BinaryOp::OP_AND: return 3;
        case BinaryOp::OP_ADD: case BinaryOp::OP_SUB: case BinaryOp::OP_OR: return 2;
        case BinaryOp::OP_EQ: case BinaryOp::OP_NE: case BinaryOp::OP_LT: case BinaryOp::OP_LE: case BinaryOp::OP_GT: case BinaryOp::OP_GE: return 1;
        default: return 0;
    }
}

static int get_expr_precedence(ExpressionNode* expr) {
    if (auto* bin = dynamic_cast<BinaryExpressionNode*>(expr)) {
        return get_precedence(bin->op);
    }
    if (dynamic_cast<UnaryExpressionNode*>(expr)) {
        return 4;
    }
    return 10;
}

void CodeGenerator::visit(BinaryExpressionNode& n) {
    int my_prec = get_precedence(n.op);
    
    if (n.op == BinaryOp::OP_DIV_REAL) {
        bool left_is_real = (get_expr_type(n.left.get()) == DataType::TY_REAL);
        bool need_cast = !left_is_real && (get_expr_type(n.right.get()) != DataType::TY_REAL);
        if (need_cast) output << "(double)(";
        bool need_left_paren = !need_cast && (get_expr_precedence(n.left.get()) < my_prec);
        if (need_left_paren) output << "(";
        n.left->accept(*this);
        if (need_left_paren) output << ")";
        if (need_cast) output << ")";
        output << " / ";
        bool need_right_paren = get_expr_precedence(n.right.get()) <= my_prec;
        if (need_right_paren) output << "(";
        n.right->accept(*this);
        if (need_right_paren) output << ")";
        return;
    }
    
    bool need_left_paren = get_expr_precedence(n.left.get()) < my_prec;
    if (need_left_paren) output << "(";
    n.left->accept(*this);
    if (need_left_paren) output << ")";
    
    output << " " << c_operator(n.op) << " ";
    
    bool need_right_paren = get_expr_precedence(n.right.get()) <= my_prec;
    if (need_right_paren) output << "(";
    n.right->accept(*this);
    if (need_right_paren) output << ")";
}

void CodeGenerator::visit(UnaryExpressionNode& n) {
    if (n.op == UnaryOp::UOP_NOT) {
        if (is_boolean_expr(
                n.operand.get(),
                [this](const std::string& name) { return lookup_type_info(name); })) {
            output << "!";
        } else {
            output << "~";
        }
    } else if (n.op == UnaryOp::UOP_NEGATE) {
        output << "-";
    }
    output << " ";
    
    bool need_paren = dynamic_cast<BinaryExpressionNode*>(n.operand.get()) != nullptr;
    if (need_paren) output << "(";
    if (n.operand) {
        n.operand->accept(*this);
    } else {
        output << "0";
    }
    if (need_paren) output << ")";
}

static bool has_side_effect(ExpressionNode* expr) {
    if (!expr) return false;
    if (dynamic_cast<FunctionCallNode*>(expr)) return true;
    if (auto* bin = dynamic_cast<BinaryExpressionNode*>(expr)) {
        return has_side_effect(bin->left.get()) || has_side_effect(bin->right.get());
    }
    if (auto* un = dynamic_cast<UnaryExpressionNode*>(expr)) {
        return has_side_effect(un->operand.get());
    }
    return false;
}

bool CodeGenerator::call_has_side_effects(const std::vector<std::unique_ptr<ExpressionNode>>& arguments) const {
    for (const auto& arg : arguments) {
        if (has_side_effect(arg.get())) {
            return true;
        }
    }
    return false;
}

void CodeGenerator::emit_call_argument_bindings(
    const std::string& callable_name,
    const std::vector<std::unique_ptr<ExpressionNode>>& arguments,
    int base) {
    const auto* params = lookup_callable_params(callable_name);

    for (int i = static_cast<int>(arguments.size()) - 1; i >= 0; --i) {
        indent();
        bool is_ref =
            params && static_cast<size_t>(i) < params->size() && (*params)[i].is_reference;
        ResolvedType arg_type =
            resolve_expr_type(arguments[i].get(), [this](const std::string& name) { return lookup_type_info(name); });
        std::string temp_name = "_tv" + std::to_string(base + i);

        if (is_ref) {
            auto* arg_ident = dynamic_cast<IdentifierNode*>(arguments[i].get());
            if (arg_ident && is_ref_param(arg_ident->name)) {
                output << temp_name << " = " << arg_ident->name << ";\n";
            } else {
                output << temp_name << " = &";
                arguments[i]->accept(*this);
                output << ";\n";
            }
        } else {
            output << temp_name << " = ";
            arguments[i]->accept(*this);
            output << ";\n";
        }
    }
}

void CodeGenerator::emit_call_argument_list(
    const std::string& callable_name,
    const std::vector<std::unique_ptr<ExpressionNode>>& arguments) {
    const auto* params = lookup_callable_params(callable_name);

    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) {
            output << ", ";
        }

        bool is_ref = params && i < params->size() && (*params)[i].is_reference;
        if (is_ref) {
            auto* arg_ident = dynamic_cast<IdentifierNode*>(arguments[i].get());
            if (arg_ident && is_ref_param(arg_ident->name)) {
                output << arg_ident->name;
                continue;
            }
            output << "&";
        }

        arguments[i]->accept(*this);
    }
}

void CodeGenerator::visit(FunctionCallNode& n) {
    if (call_has_side_effects(n.arguments)) {
        output << "(";
        std::vector<std::string> temp_names(n.arguments.size());
        bool first = true;

        for (int i = static_cast<int>(n.arguments.size()) - 1; i >= 0; --i) {
            bool is_ref = false;
            if (const auto* params = lookup_callable_params(n.func_name)) {
                is_ref = static_cast<size_t>(i) < params->size() && (*params)[i].is_reference;
            }

            ResolvedType arg_type =
                resolve_expr_type(n.arguments[i].get(), [this](const std::string& name) { return lookup_type_info(name); });
            temp_names[i] = reserve_temp_name(arg_type, is_ref);

            if (!first) {
                output << ", ";
            }
            first = false;
            output << temp_names[i] << " = ";
            if (is_ref) {
                auto* arg_ident = dynamic_cast<IdentifierNode*>(n.arguments[i].get());
                if (arg_ident && is_ref_param(arg_ident->name)) {
                    output << arg_ident->name;
                } else {
                    output << "&";
                    n.arguments[i]->accept(*this);
                }
            } else {
                n.arguments[i]->accept(*this);
            }
        }

        if (!n.arguments.empty()) {
            output << ", ";
        }
        output << emitted_callable_name(n.func_name) << "(";
        for (size_t i = 0; i < temp_names.size(); ++i) {
            if (i > 0) output << ", ";
            output << temp_names[i];
        }
        output << "))";
    } else {
        output << emitted_callable_name(n.func_name) << "(";
        emit_call_argument_list(n.func_name, n.arguments);
        output << ")";
    }
}

void CodeGenerator::visit(AssignmentNode& n) {
    indent();
    if (!current_func_name.empty()) {
        if (auto* target = dynamic_cast<IdentifierNode*>(n.target.get())) {
            if (target->name == current_func_name) {
                output << "_retval = ";
                n.value->accept(*this);
                output << ";\n";
                return;
            }
        }
    }
    n.target->accept(*this);
    output << " = ";
    n.value->accept(*this);
    output << ";\n";
}

void CodeGenerator::visit(CompoundStatementNode& n) {
    output << "{\n";
    indent_level++;
    for (const auto& stmt : n.statements) {
        if (stmt) stmt->accept(*this);
    }
    indent_level--;
    indent();
    output << "}\n";
}

void CodeGenerator::visit(IfStatementNode& n) {
    indent();
    output << "if (";
    n.condition->accept(*this);
    output << ") ";
    
    bool then_is_compound = (n.then_branch && dynamic_cast<CompoundStatementNode*>(n.then_branch.get()));
    if (!then_is_compound) {
        output << "{\n";
        indent_level++;
    }
    
    if (n.then_branch) {
        n.then_branch->accept(*this);
    }
    
    if (!then_is_compound) {
        indent_level--;
        indent();
        output << "}";
    }
    
    if (n.else_branch) {
        output << " else ";
        bool else_is_compound = dynamic_cast<CompoundStatementNode*>(n.else_branch.get());
        if (!else_is_compound) {
            output << "{\n";
            indent_level++;
        }
        n.else_branch->accept(*this);
        if (!else_is_compound) {
            indent_level--;
            indent();
            output << "}";
        }
    }
    output << "\n";
}

void CodeGenerator::visit(WhileStatementNode& n) {
    indent();
    output << "while (";
    n.condition->accept(*this);
    output << ") ";
    
    bool body_is_compound = (n.body && dynamic_cast<CompoundStatementNode*>(n.body.get()));
    if (!body_is_compound) {
        output << "{\n";
        indent_level++;
    }
    
    n.body->accept(*this);
    
    if (!body_is_compound) {
        indent_level--;
        indent();
        output << "}\n";
    } else {
        output << "\n";
    }
}

void CodeGenerator::visit(ForStatementNode& n) {
    indent();
    output << "for (" << n.loop_var << " = ";
    n.start->accept(*this);
    output << "; " << n.loop_var << (n.is_downto ? " >= " : " <= ");
    n.end->accept(*this);
    output << "; " << n.loop_var << (n.is_downto ? "--" : "++") << ") ";
    
    bool body_is_compound = (n.body && dynamic_cast<CompoundStatementNode*>(n.body.get()));
    if (!body_is_compound) {
        output << "{\n";
        indent_level++;
    }
    
    n.body->accept(*this);
    
    if (!body_is_compound) {
        indent_level--;
        indent();
        output << "}\n";
    } else {
        output << "\n";
    }
}

void CodeGenerator::visit(ProcedureCallNode& n) {
    if (n.proc_name == "break") {
        indent();
        output << "break;\n";
        return;
    }
    if (n.proc_name == "read" || n.proc_name == "readln") {
        for (size_t ai = 0; ai < n.arguments.size(); ai++) {
            if (ai > 0) indent();
            auto* arg = n.arguments[ai].get();
            auto* arg_id = dynamic_cast<IdentifierNode*>(arg);
            bool is_func_return = false;
            if (arg_id) {
                if (arg_id->name == current_func_name && !current_func_name.empty()) {
                    is_func_return = true;
                } else {
                    auto se = g_symbol_table.lookup(arg_id->name);
                    if (se && se->is_subprogram()) {
                        is_func_return = true;
                    }
                }
            }
            DataType read_type = DataType::TY_INTEGER;
            if (arg_id) {
                read_type = get_identifier_type(arg_id->name);
                auto se = g_symbol_table.lookup(arg_id->name);
                if (se) {
                    if (se->is_subprogram()) read_type = se->return_type;
                    else read_type = se->type;
                }
            } else if (dynamic_cast<ArrayAccessNode*>(arg)) {
                read_type = get_expr_type(arg);
            }
            std::string fmt = c_format_specifier(read_type, true);
            if (is_func_return) {
                output << "{ " << c_type(read_type) << " _rv; scanf(\"" << fmt << "\", &_rv); _retval = _rv; }\n";
            } else if (arg_id && is_ref_param(arg_id->name)) {
                output << "scanf(\"" << fmt << "\", " << arg_id->name << ");\n";
            } else {
                output << "scanf(\"" << fmt << "\", &";
                if (arg_id) {
                    output << arg_id->name;
                } else {
                    arg->accept(*this);
                }
                output << ");\n";
            }
        }
        return;
    }
    
    if (call_has_side_effects(n.arguments)) {
        std::vector<std::string> temp_names(n.arguments.size());
        for (int i = static_cast<int>(n.arguments.size()) - 1; i >= 0; --i) {
            bool is_ref = false;
            if (const auto* params = lookup_callable_params(n.proc_name)) {
                is_ref = static_cast<size_t>(i) < params->size() && (*params)[i].is_reference;
            }

            ResolvedType arg_type =
                resolve_expr_type(n.arguments[i].get(), [this](const std::string& name) { return lookup_type_info(name); });
            temp_names[i] = reserve_temp_name(arg_type, is_ref);
        }

        for (int i = static_cast<int>(n.arguments.size()) - 1; i >= 0; --i) {
            indent();
            bool is_ref = false;
            if (const auto* params = lookup_callable_params(n.proc_name)) {
                is_ref = static_cast<size_t>(i) < params->size() && (*params)[i].is_reference;
            }

            output << temp_names[i] << " = ";
            if (is_ref) {
                auto* arg_ident = dynamic_cast<IdentifierNode*>(n.arguments[i].get());
                if (arg_ident && is_ref_param(arg_ident->name)) {
                    output << arg_ident->name;
                } else {
                    output << "&";
                    n.arguments[i]->accept(*this);
                }
            } else {
                n.arguments[i]->accept(*this);
            }
            output << ";\n";
        }
        indent();
        output << emitted_callable_name(n.proc_name) << "(";
        for (size_t i = 0; i < temp_names.size(); ++i) {
            if (i > 0) output << ", ";
            output << temp_names[i];
        }
        output << ");\n";
    } else {
        indent();
        output << emitted_callable_name(n.proc_name) << "(";
        emit_call_argument_list(n.proc_name, n.arguments);
        output << ");\n";
    }
}

void CodeGenerator::visit(WriteStatementNode& n) {
    indent();
    std::string write_fmt = "";
    
    auto get_write_fmt = [this](ExpressionNode* expr) -> std::string {
        if (dynamic_cast<StringLiteralNode*>(expr)) return "%s";
        if (auto* id = dynamic_cast<IdentifierNode*>(expr)) {
            if (string_consts.count(id->name)) {
                return "%s";
            }
        }
        DataType t = get_expr_type(expr);
        return c_format_specifier(t);
    };
    
    if (!n.values.empty()) {
        output << "printf(\"";
        for (size_t i = 0; i < n.values.size(); i++) {
            output << get_write_fmt(n.values[i].get());
        }
        output << "\", ";
        for (size_t i = 0; i < n.values.size(); i++) {
            if (i > 0) output << ", ";
            n.values[i]->accept(*this);
        }
        output << ");\n";
    } else if (n.value) {
        output << "printf(\"" << get_write_fmt(n.value.get()) << "\", ";
        n.value->accept(*this);
        output << ");\n";
    } else {
        output << "printf(\"\\n\");\n";
    }
}

void CodeGenerator::visit(VariableDeclarationNode& n) {
    remember_symbol_type(n.var_name, n.type, n.array_info, &n.record_info);
    
    if (n.is_const) {
        indent();
        bool is_string = (n.init_value && dynamic_cast<StringLiteralNode*>(n.init_value.get()));
        if (is_string) {
            string_consts.insert(n.var_name);
            output << "const char* " << n.var_name << " = ";
        } else {
            output << "const " << c_decl_type(n.type, &n.record_info) << " " << n.var_name << " = ";
        }
        if (n.init_value) {
            n.init_value->accept(*this);
        } else {
            output << "0";
        }
        output << ";\n";
    } else {
        emit_decl_for_variable(n);
    }
}

void CodeGenerator::visit(FunctionDeclarationNode& n) {
    func_params[n.func_name] = n.parameters;
    
    std::string emit_name = (n.func_name == "main") ? "_pascal_main" : n.func_name;
    output << c_type(n.return_type) << " " << emit_name << "(";
    
    bool first = true;
    for (const auto& param : n.parameters) {
        if (!first) output << ", ";
        first = false;
        
        if (param.is_reference) {
            output << c_decl_type(param.type, &param.record_info) << "* " << param.name;
        } else {
            output << c_decl_type(param.type, &param.record_info) << " " << param.name;
        }
    }
    
    output << ") {\n";
    
    std::string prev_func_name = current_func_name;
    current_func_name = n.func_name;
    
    ref_params.clear();
    for (const auto& param : n.parameters) {
        if (param.is_reference) {
            ref_params.insert(param.name);
        }
        remember_symbol_type(param.name, param.type, param.array_info, &param.record_info);
    }
    
    indent_level++;
    reset_temp_declarations();
    std::ostringstream body_stream;
    auto* old_buf = output.rdbuf();
    static_cast<std::ostream&>(output).rdbuf(body_stream.rdbuf());

    auto it = func_local_vars.find(n.func_name);
    if (it != func_local_vars.end()) {
        for (auto* var : it->second) {
            remember_symbol_type(var->var_name, var->type, var->array_info, &var->record_info);
            emit_decl_for_variable(*var);
        }
    }
    
    if (n.return_type != DataType::TY_VOID) {
        indent();
        output << c_type(n.return_type) << " _retval = 0;\n";
    }
    
    if (n.body) {
        if (auto* compound = dynamic_cast<CompoundStatementNode*>(n.body.get())) {
            for (const auto& stmt : compound->statements) {
                if (stmt) stmt->accept(*this);
            }
        } else {
            n.body->accept(*this);
        }
    }
    if (n.return_type != DataType::TY_VOID) {
        indent_level++;
        indent();
        output << "return _retval;\n";
        indent_level--;
    }

    static_cast<std::ostream&>(output).rdbuf(old_buf);
    emit_temp_declarations();
    output << body_stream.str();

    indent_level--;
    current_func_name = prev_func_name;
    
    output << "}\n\n";
}

DataType CodeGenerator::get_identifier_type(const std::string& name) {
    auto type_info = lookup_type_info(name);
    return type_info ? type_info->effective_type() : DataType::TY_INTEGER;
}

DataType CodeGenerator::get_expr_type(ExpressionNode* expr) {
    return resolve_expr_type(
               expr,
               [this](const std::string& name) { return lookup_type_info(name); })
        .type;
}

bool CodeGenerator::is_ref_param(const std::string& name) {
    return ref_params.find(name) != ref_params.end();
}

void CodeGenerator::visit(ProgramNode& /*n*/) {
    // ProgramNode 由 generate() 函数处理
}

} // namespace pascal_s
