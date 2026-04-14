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
    
    if (program->main_body) {
        for (const auto& stmt : program->main_body->statements) {
            if (stmt) stmt->accept(*this);
        }
    }
    
    indent();
    output << "return 0;\n";
    
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

static bool is_bool_expr(ExpressionNode* expr) {
    if (!expr) return false;
    if (dynamic_cast<BooleanLiteralNode*>(expr)) {
        return true;
    }
    if (dynamic_cast<IntegerLiteralNode*>(expr)) {
        return false;
    }
    if (auto* bin = dynamic_cast<BinaryExpressionNode*>(expr)) {
        if (bin->op >= BinaryOp::OP_EQ && bin->op <= BinaryOp::OP_GE) return true;
        if (bin->op == BinaryOp::OP_AND || bin->op == BinaryOp::OP_OR) return true;
        return false;
    }
    if (auto* un = dynamic_cast<UnaryExpressionNode*>(expr)) {
        return is_bool_expr(un->operand.get());
    }
    if (auto* id = dynamic_cast<IdentifierNode*>(expr)) {
        auto sym = g_symbol_table.lookup(id->name);
        if (sym) return sym->type == DataType::TY_BOOLEAN;
        return false;
    }
    if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr)) {
        auto sym = g_symbol_table.lookup(arr->array_name);
        if (sym) return sym->array_info.element_type == DataType::TY_BOOLEAN || sym->type == DataType::TY_BOOLEAN;
        return false;
    }
    if (auto* rec = dynamic_cast<RecordAccessNode*>(expr)) {
        if (auto* base = dynamic_cast<IdentifierNode*>(rec->record_expr.get())) {
            auto sym = g_symbol_table.lookup(base->name);
            if (sym) {
                for (const auto& field : sym->record_info.fields) {
                    if (field.name == rec->field_name) return field.type == DataType::TY_BOOLEAN;
                }
            }
        }
        return false;
    }
    return false;
}

void CodeGenerator::visit(UnaryExpressionNode& n) {
    if (n.op == UnaryOp::UOP_NOT) {
        if (is_bool_expr(n.operand.get())) {
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

void CodeGenerator::visit(FunctionCallNode& n) {
    bool has_side_effects = false;
    for (const auto& arg : n.arguments) {
        if (has_side_effect(arg.get())) {
            has_side_effects = true;
            break;
        }
    }
    
    if (has_side_effects) {
        int base = temp_var_counter;
        temp_var_counter += n.arguments.size();
        
        output << "({\n";
        indent_level++;
        
        auto it = func_params.find(n.func_name);
        
        for (int i = static_cast<int>(n.arguments.size()) - 1; i >= 0; i--) {
            indent();
            bool is_ref = (it != func_params.end() && static_cast<size_t>(i) < it->second.size() && it->second[i].is_reference);
            DataType arg_type = get_expr_type(n.arguments[i].get());
            std::string c_type_str = c_type(arg_type);
            
            if (is_ref) {
                auto* arg_ident = dynamic_cast<IdentifierNode*>(n.arguments[i].get());
                if (arg_ident && is_ref_param(arg_ident->name)) {
                    output << c_type_str << " _tv" << (base + i) << " = " << arg_ident->name << ";\n";
                } else {
                    output << c_type_str << "* _tv" << (base + i) << " = &";
                    n.arguments[i]->accept(*this);
                    output << ";\n";
                }
            } else {
                output << c_type_str << " _tv" << (base + i) << " = ";
                n.arguments[i]->accept(*this);
                output << ";\n";
            }
        }
        
        indent();
        output << (n.func_name == "main" ? "_pascal_main" : n.func_name) << "(";
        for (size_t i = 0; i < n.arguments.size(); i++) {
            if (i > 0) output << ", ";
            bool is_ref = (it != func_params.end() && i < it->second.size() && it->second[i].is_reference);
            if (is_ref) {
                output << "_tv" << (base + i);
            } else {
                output << "_tv" << (base + i);
            }
        }
        output << ");\n";
        
        indent_level--;
        indent();
        output << "})";
    } else {
        output << (n.func_name == "main" ? "_pascal_main" : n.func_name) << "(";
        bool first = true;
        auto it = func_params.find(n.func_name);
        for (size_t i = 0; i < n.arguments.size(); i++) {
            if (!first) output << ", ";
            first = false;
            
            bool is_ref = false;
            if (it != func_params.end() && i < it->second.size()) {
                is_ref = it->second[i].is_reference;
            }
            
            if (is_ref) {
                auto* arg_ident = dynamic_cast<IdentifierNode*>(n.arguments[i].get());
                if (arg_ident && is_ref_param(arg_ident->name)) {
                    output << arg_ident->name;
                    continue;
                } else {
                    output << "&";
                }
            }
            n.arguments[i]->accept(*this);
        }
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
    indent();
    if (n.proc_name == "break") {
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
    
    bool has_side_effects = false;
    for (const auto& arg : n.arguments) {
        if (has_side_effect(arg.get())) {
            has_side_effects = true;
            break;
        }
    }
    
    if (has_side_effects) {
        int base = temp_var_counter;
        temp_var_counter += n.arguments.size();
        
        output << "({\n";
        indent_level++;
        
        auto it = func_params.find(n.proc_name);
        
        for (int i = static_cast<int>(n.arguments.size()) - 1; i >= 0; i--) {
            indent();
            bool is_ref = (it != func_params.end() && static_cast<size_t>(i) < it->second.size() && it->second[i].is_reference);
            DataType arg_type = get_expr_type(n.arguments[i].get());
            std::string c_type_str = c_type(arg_type);
            
            if (is_ref) {
                auto* arg_ident = dynamic_cast<IdentifierNode*>(n.arguments[i].get());
                if (arg_ident && is_ref_param(arg_ident->name)) {
                    output << c_type_str << " _tv" << (base + i) << " = " << arg_ident->name << ";\n";
                } else {
                    output << c_type_str << "* _tv" << (base + i) << " = &";
                    n.arguments[i]->accept(*this);
                    output << ";\n";
                }
            } else {
                output << c_type_str << " _tv" << (base + i) << " = ";
                n.arguments[i]->accept(*this);
                output << ";\n";
            }
        }
        
        indent();
        output << (n.proc_name == "main" ? "_pascal_main" : n.proc_name) << "(";
        for (size_t i = 0; i < n.arguments.size(); i++) {
            if (i > 0) output << ", ";
            bool is_ref = (it != func_params.end() && i < it->second.size() && it->second[i].is_reference);
            if (is_ref) {
                output << "_tv" << (base + i);
            } else {
                output << "_tv" << (base + i);
            }
        }
        output << ");\n";
        
        indent_level--;
        indent();
        output << "});\n";
    } else {
        output << (n.proc_name == "main" ? "_pascal_main" : n.proc_name) << "(";
        bool first = true;
        auto it = func_params.find(n.proc_name);
        for (size_t i = 0; i < n.arguments.size(); i++) {
            if (!first) output << ", ";
            first = false;
            
            bool is_ref = false;
            if (it != func_params.end() && i < it->second.size()) {
                is_ref = it->second[i].is_reference;
            }
            
            if (is_ref) {
                auto* arg_ident = dynamic_cast<IdentifierNode*>(n.arguments[i].get());
                if (arg_ident && is_ref_param(arg_ident->name)) {
                    output << arg_ident->name;
                    continue;
                } else {
                    output << "&";
                }
            }
            n.arguments[i]->accept(*this);
        }
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
    if ((n.type == DataType::TY_ARRAY || n.is_array) && n.array_info.element_type != DataType::TY_UNKNOWN) {
        var_types[n.var_name] = n.array_info.element_type;
    } else {
        var_types[n.var_name] = n.type;
    }
    
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
        } else if (n.type == DataType::TY_RECORD) {
            output << c_decl_type(n.type, &n.record_info) << " " << n.var_name << ";\n";
        } else {
            output << c_type(n.type) << " " << n.var_name << ";\n";
        }
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
        var_types[param.name] = param.type;
    }
    
    indent_level++;
    
    auto it = func_local_vars.find(n.func_name);
    if (it != func_local_vars.end()) {
        for (auto* var : it->second) {
            if ((var->type == DataType::TY_ARRAY || var->is_array) && var->array_info.element_type != DataType::TY_UNKNOWN) {
                var_types[var->var_name] = var->array_info.element_type;
            } else {
                var_types[var->var_name] = var->type;
            }
            
            indent();
            if (var->type == DataType::TY_ARRAY || var->is_array) {
                std::string elem_type = "int";
                if (var->array_info.element_type != DataType::TY_UNKNOWN) {
                    const RecordInfo* record_info =
                        var->array_info.element_type == DataType::TY_RECORD ? &var->record_info : nullptr;
                    elem_type = c_decl_type(var->array_info.element_type, record_info);
                }
                
                output << elem_type << " " << var->var_name;
                
                if (!var->array_info.dimensions.empty()) {
                    for (const auto& dim : var->array_info.dimensions) {
                        output << "[" << dim.size() << "]";
                    }
                } else if (var->array_info.upper_bound >= var->array_info.lower_bound) {
                    int size = var->array_info.upper_bound - var->array_info.lower_bound + 1;
                    output << "[" << size << "]";
                } else {
                    output << "[1]";
                }
                output << ";\n";
            } else if (var->type == DataType::TY_RECORD) {
                output << c_decl_type(var->type, &var->record_info) << " " << var->var_name << ";\n";
            } else {
                output << c_type(var->type) << " " << var->var_name << ";\n";
            }
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
    indent_level--;
    
    current_func_name = prev_func_name;
    
    output << "}\n\n";
}

DataType CodeGenerator::get_identifier_type(const std::string& name) {
    auto it = var_types.find(name);
    if (it != var_types.end()) {
        return it->second;
    }
    auto sym = g_symbol_table.lookup(name);
    if (sym) {
        if (sym->is_subprogram()) return sym->return_type;
        return sym->type;
    }
    return DataType::TY_INTEGER;
}

DataType CodeGenerator::get_expr_type(ExpressionNode* expr) {
    if (!expr) return DataType::TY_INTEGER;
    if (dynamic_cast<RealLiteralNode*>(expr)) return DataType::TY_REAL;
    if (dynamic_cast<BooleanLiteralNode*>(expr)) return DataType::TY_BOOLEAN;
    if (dynamic_cast<CharLiteralNode*>(expr)) return DataType::TY_CHAR;
    if (dynamic_cast<StringLiteralNode*>(expr)) return DataType::TY_CHAR;
    if (auto* id = dynamic_cast<IdentifierNode*>(expr)) {
        return get_identifier_type(id->name);
    }
    if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr)) {
        auto it = var_types.find(arr->array_name);
        if (it != var_types.end() && it->second != DataType::TY_ARRAY && it->second != DataType::TY_UNKNOWN) {
            return it->second;
        }
        auto sym = g_symbol_table.lookup(arr->array_name);
        if (sym && sym->array_info.element_type != DataType::TY_UNKNOWN) {
            return sym->array_info.element_type;
        }
        if (sym && sym->type != DataType::TY_ARRAY && sym->type != DataType::TY_UNKNOWN) {
            return sym->type;
        }
        return DataType::TY_INTEGER;
    }
    if (auto* record = dynamic_cast<RecordAccessNode*>(expr)) {
        if (auto* base_ident = dynamic_cast<IdentifierNode*>(record->record_expr.get())) {
            auto sym = g_symbol_table.lookup(base_ident->name);
            if (sym) {
                for (const auto& field : sym->record_info.fields) {
                    if (field.name == record->field_name) {
                        return field.type;
                    }
                }
            }
        } else if (auto* base_record = dynamic_cast<RecordAccessNode*>(record->record_expr.get())) {
            DataType base_type = get_expr_type(base_record);
            if (base_type == DataType::TY_RECORD) {
                if (auto* base_ident2 = dynamic_cast<IdentifierNode*>(base_record->record_expr.get())) {
                    auto sym = g_symbol_table.lookup(base_ident2->name);
                    if (sym) {
                        const RecordInfo* info = &sym->record_info;
                        for (const auto& segment : info->fields) {
                            if (segment.name == base_record->field_name && segment.record_info) {
                                info = segment.record_info.get();
                                break;
                            }
                        }
                        for (const auto& field : info->fields) {
                            if (field.name == record->field_name) {
                                return field.type;
                            }
                        }
                    }
                }
            }
        } else if (auto* base_arr = dynamic_cast<ArrayAccessNode*>(record->record_expr.get())) {
            auto sym = g_symbol_table.lookup(base_arr->array_name);
            if (sym) {
                for (const auto& field : sym->record_info.fields) {
                    if (field.name == record->field_name) {
                        return field.type;
                    }
                }
            }
        }
        return DataType::TY_UNKNOWN;
    }
    if (auto* un = dynamic_cast<UnaryExpressionNode*>(expr)) {
        return get_expr_type(un->operand.get());
    }
    if (auto* bin = dynamic_cast<BinaryExpressionNode*>(expr)) {
        if (get_expr_type(bin->left.get()) == DataType::TY_REAL || get_expr_type(bin->right.get()) == DataType::TY_REAL) {
            return DataType::TY_REAL;
        }
        return get_expr_type(bin->left.get());
    }
    if (auto* fc = dynamic_cast<FunctionCallNode*>(expr)) {
        auto sym = g_symbol_table.lookup(fc->func_name);
        if (sym) return sym->return_type;
        return DataType::TY_INTEGER;
    }
    return DataType::TY_INTEGER;
}

bool CodeGenerator::is_ref_param(const std::string& name) {
    return ref_params.find(name) != ref_params.end();
}

void CodeGenerator::visit(ProgramNode& /*n*/) {
    // ProgramNode 由 generate() 函数处理
}

} // namespace pascal_s
