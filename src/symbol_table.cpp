#include "symbol_table.h"

namespace pascal_s {

SymbolTable::SymbolTable() { 
    enter_scope();  // 全局作用域
    current_offset = 0;
}

// 进入新作用域时重新从 0 计数 offset；课程设计里它只用于语义层面的“位置”概念。
void SymbolTable::enter_scope() {
    scopes.emplace_back();
    current_offset = 0;
}

void SymbolTable::exit_scope() {
    if (scopes.size() > 1) {  // 保留全局作用域
        scopes.pop_back();
    }
}

int SymbolTable::current_scope_level() const { 
    return static_cast<int>(scopes.size()) - 1; 
}

// insert 只向当前作用域写入，重复声明由当前层判重负责。
bool SymbolTable::insert(const std::string& name, DataType type, 
            bool is_const, bool is_ref,
            const ArrayInfo& arr_info) {
    if (exists_in_current_scope(name)) return false;
    
    auto entry = std::make_shared<SymbolEntry>(
        name, type, current_scope_level(), current_offset++);
    entry->is_const = is_const;
    entry->is_reference = is_ref;
    entry->array_info = arr_info;
    scopes.back()[name] = entry;
    return true;
}

// lookup 按“最近作用域优先”查找，保证局部声明可以屏蔽外层同名标识符。
std::shared_ptr<SymbolEntry> SymbolTable::lookup(const std::string& name) {
    for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; --i) {
        auto it = scopes[i].find(name);
        if (it != scopes[i].end()) return it->second;
    }
    return nullptr;
}

std::shared_ptr<const SymbolEntry> SymbolTable::lookup(const std::string& name) const {
    for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; --i) {
        auto it = scopes[i].find(name);
        if (it != scopes[i].end()) return it->second;
    }
    return nullptr;
}

// 检查是否在当前作用域存在
bool SymbolTable::exists_in_current_scope(const std::string& name) {
    return scopes.back().find(name) != scopes.back().end();
}

// 添加函数/过程到符号表
void SymbolTable::add_function(const std::string& name, DataType return_type,
                  const std::vector<ParameterInfo>& params, bool is_proc) {
    auto entry = std::make_shared<SymbolEntry>(
        name, is_proc ? DataType::TY_PROCEDURE : DataType::TY_FUNCTION, 0, 0);
    entry->return_type = return_type;
    entry->params = params;
    scopes[0][name] = entry;
}

// 添加参数到当前作用域
void SymbolTable::add_parameter(const std::string& name, DataType type, bool is_ref) {
    insert(name, type, false, is_ref);
}

// 获取当前作用域的所有符号
const std::unordered_map<std::string, std::shared_ptr<SymbolEntry>>& SymbolTable::current_scope() const {
    return scopes.back();
}

// 重置符号表
void SymbolTable::clear() {
    scopes.clear();
    enter_scope();
    current_offset = 0;
}

// 类型兼容性检查
bool TypeSystem::is_compatible(DataType from, DataType to) {
    if (from == to) return true;
    // integer 可隐式转换为 real
    if (from == DataType::TY_INTEGER && to == DataType::TY_REAL) return true;
    return false;
}

// 二元运算结果类型推断
DataType TypeSystem::infer_binary_result(BinaryOp op, DataType left, DataType right) {
    // 关系运算返回 boolean
    if (op >= BinaryOp::OP_EQ && op <= BinaryOp::OP_GE) {
        return DataType::TY_BOOLEAN;
    }
    // 逻辑运算
    if (op == BinaryOp::OP_AND || op == BinaryOp::OP_OR) {
        return DataType::TY_BOOLEAN;
    }
    // 算术运算
    if (left == DataType::TY_REAL || right == DataType::TY_REAL) {
        return DataType::TY_REAL;
    }
    // Pascal 中的 '/' 始终表示实数除法，因此结果类型固定提升到 real。
    if (op == BinaryOp::OP_DIV_REAL) {
        return DataType::TY_REAL;
    }
    // div/mod 返回 integer
    if (op == BinaryOp::OP_DIV || op == BinaryOp::OP_MOD) {
        return DataType::TY_INTEGER;
    }
    // 默认 integer
    return DataType::TY_INTEGER;
}

// 类型名称 (用于代码生成)
std::string TypeSystem::to_c_type(DataType t) {
    switch (t) {
        case DataType::TY_INTEGER: return "int";
        case DataType::TY_REAL: return "double";
        case DataType::TY_BOOLEAN: return "int";  // 当前类型系统默认把 boolean 落成 int，后端再决定是否引入 stdbool.h。
        case DataType::TY_CHAR: return "char";
        default: return "void";
    }
}

// 从 Pascal 类型关键字获取 DataType
DataType TypeSystem::from_keyword(const std::string& kw) {
    if (kw == "integer") return DataType::TY_INTEGER;
    if (kw == "real") return DataType::TY_REAL;
    if (kw == "boolean") return DataType::TY_BOOLEAN;
    if (kw == "char") return DataType::TY_CHAR;
    return DataType::TY_UNKNOWN;
}

} // namespace pascal_s
