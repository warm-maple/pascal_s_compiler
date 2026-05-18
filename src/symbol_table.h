#pragma once
#include "ast.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

namespace pascal_s {

// 符号表条目：一处声明在语义阶段落成一条记录，后续查找和类型检查都围绕它展开。
class SymbolEntry {
public:
    std::string name;
    DataType type;
    
    // 作用域信息
    int scope_level;
    // 这里的 offset 只表达当前作用域内的相对位置属性，不涉及机器级重定位。
    int offset;
    
    // 额外信息 (根据类型)
    bool is_const;
    bool is_reference;  // 仅参数
    ArrayInfo array_info;
    RecordInfo record_info;
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

// 符号表采用作用域栈：进入函数/过程时压栈，退出时弹栈；lookup 总是从内层向外层查找。
class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, std::shared_ptr<SymbolEntry>>> scopes;
    int current_offset;
    
public:
    SymbolTable();
    
    // 作用域管理
    void enter_scope();
    void exit_scope();
    int current_scope_level() const;
    
    // 符号插入
    bool insert(const std::string& name, DataType type, 
                bool is_const = false, bool is_ref = false,
                const ArrayInfo& arr_info = ArrayInfo{});
    
    // 查找符号 (从当前作用域向外)
    std::shared_ptr<SymbolEntry> lookup(const std::string& name);
    std::shared_ptr<const SymbolEntry> lookup(const std::string& name) const;
    
    // 检查是否在当前作用域存在
    bool exists_in_current_scope(const std::string& name);
    
    // 添加函数/过程到符号表
    void add_function(const std::string& name, DataType return_type,
                      const std::vector<ParameterInfo>& params, bool is_proc = false);
    
    // 添加参数到当前作用域
    void add_parameter(const std::string& name, DataType type, bool is_ref = false);
    
    // 获取当前作用域的所有符号
    const std::unordered_map<std::string, std::shared_ptr<SymbolEntry>>& current_scope() const;
    
    // 重置符号表
    void clear();
};

// 类型系统工具类
class TypeSystem {
public:
    // 类型兼容性检查
    static bool is_compatible(DataType from, DataType to);
    
    // 二元运算结果类型推断
    static DataType infer_binary_result(BinaryOp op, DataType left, DataType right);
    
    // 类型名称 (用于代码生成)
    static std::string to_c_type(DataType t);
    
    // 从 Pascal 类型关键字获取 DataType
    static DataType from_keyword(const std::string& kw);
};

} // namespace pascal_s
