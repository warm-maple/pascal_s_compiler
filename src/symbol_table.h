#pragma once
#include "ast.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

namespace pascal_s {

// 符号表条目
class SymbolEntry {
public:
    std::string name;
    DataType type;
    
    // 作用域信息
    int scope_level;
    int offset;  // 栈帧偏移
    
    // 额外信息 (根据类型)
    bool is_const;
    bool is_reference;  // 仅参数
    ArrayInfo array_info;
    std::vector<ParameterInfo> params;  // 仅函数/过程
    DataType return_type;  // 仅函数
    
    // 构造函数
    SymbolEntry(const std::string& n, DataType t, int level, int off = 0)
        : name(n), type(t), scope_level(level), offset(off),
          is_const(false), is_reference(false), return_type(DataType::TY_VOID) {}
    
    // 是否为函数/过程
    bool is_subprogram() const {
        return type == DataType::TY_FUNCTION || type == DataType::TY_PROCEDURE;
    }
};

// 符号表 (两级栈式结构)
class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, std::shared_ptr<SymbolEntry>>> scopes;
    int current_offset;
    
public:
    SymbolTable() { 
        enter_scope();  // 全局作用域
        current_offset = 0;
    }
    
    // 作用域管理
    void enter_scope() {
        scopes.emplace_back();
        current_offset = 0;
    }
    
    void exit_scope() {
        if (scopes.size() > 1) {  // 保留全局作用域
            scopes.pop_back();
        }
    }
    
    int current_scope_level() const { return static_cast<int>(scopes.size()) - 1; }
    
    // 符号插入
    bool insert(const std::string& name, DataType type, 
                bool is_const = false, bool is_ref = false,
                const ArrayInfo& arr_info = ArrayInfo{}) {
        if (exists_in_current_scope(name)) return false;
        
        auto entry = std::make_shared<SymbolEntry>(
            name, type, current_scope_level(), current_offset++);
        entry->is_const = is_const;
        entry->is_reference = is_ref;
        entry->array_info = arr_info;
        scopes.back()[name] = entry;
        return true;
    }
    
    // 查找符号 (从当前作用域向外)
    std::shared_ptr<SymbolEntry> lookup(const std::string& name) {
        for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; --i) {
            auto it = scopes[i].find(name);
            if (it != scopes[i].end()) return it->second;
        }
        return nullptr;
    }
    
    // 检查是否在当前作用域存在
    bool exists_in_current_scope(const std::string& name) {
        return scopes.back().find(name) != scopes.back().end();
    }
    
    // 添加函数/过程到符号表
    void add_function(const std::string& name, DataType return_type,
                      const std::vector<ParameterInfo>& params, bool is_proc = false) {
        auto entry = std::make_shared<SymbolEntry>(
            name, is_proc ? DataType::TY_PROCEDURE : DataType::TY_FUNCTION, 0, 0);
        entry->return_type = return_type;
        entry->params = params;
        scopes[0][name] = entry;
    }
    
    // 添加参数到当前作用域
    void add_parameter(const std::string& name, DataType type, bool is_ref = false) {
        insert(name, type, false, is_ref);
    }
    
    // 获取当前作用域的所有符号
    const std::unordered_map<std::string, std::shared_ptr<SymbolEntry>>& current_scope() const {
        return scopes.back();
    }
    
    // 重置符号表
    void clear() {
        scopes.clear();
        enter_scope();
        current_offset = 0;
    }
};

// 类型系统工具类
class TypeSystem {
public:
    // 类型兼容性检查
    static bool is_compatible(DataType from, DataType to) {
        if (from == to) return true;
        // integer 可隐式转换为 real
        if (from == DataType::TY_INTEGER && to == DataType::TY_REAL) return true;
        return false;
    }
    
    // 二元运算结果类型推断
    static DataType infer_binary_result(BinaryOp op, DataType left, DataType right) {
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
        // Pascal / 实数除法始终返回 real
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
    static std::string to_c_type(DataType t) {
        switch (t) {
            case DataType::TY_INTEGER: return "int";
            case DataType::TY_REAL: return "double";
            case DataType::TY_BOOLEAN: return "int";  // C99 无原生 bool
            case DataType::TY_CHAR: return "char";
            default: return "void";
        }
    }
    
    // 从 Pascal 类型关键字获取 DataType
    static DataType from_keyword(const std::string& kw) {
        if (kw == "integer") return DataType::TY_INTEGER;
        if (kw == "real") return DataType::TY_REAL;
        if (kw == "boolean") return DataType::TY_BOOLEAN;
        if (kw == "char") return DataType::TY_CHAR;
        return DataType::TY_UNKNOWN;
    }
};

} // namespace pascal_s
