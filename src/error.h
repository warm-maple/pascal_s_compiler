#pragma once
#include <string>
#include <vector>
#include <set>
#include <iostream>

namespace pascal_s {

enum class ErrorType {
    LEXICAL,
    SYNTAX,
    SEMANTIC,
    CODE_GENERATION
};

struct CompilerError {
    ErrorType type;
    std::string message;
    int line;
    int column;
    int length;
    bool recovered;
    
    CompilerError(ErrorType t, const std::string& msg, int l, int c, int len = 1, bool rec = false)
        : type(t), message(msg), line(l), column(c), length(len), recovered(rec) {}
};

// 错误处理器 (单例模式)
class ErrorHandler {
private:
    std::vector<CompilerError> errors;
    std::vector<std::string> source_lines; // Loaded lines
    bool in_panic_mode = false;
    std::set<std::pair<int, int>> reported_positions;  // 防雪崩：已报告位置
    
    static const int MAX_ERRORS = 50;  // 最大错误数
    
    ErrorHandler() = default;
    
public:
    static ErrorHandler& instance();
    
    void set_source_lines(const std::vector<std::string>& lines);
    
    // 词法错误
    void lexical_error(const std::string& msg, int line, int col, int length = 1);
    
    // 语法错误
    void syntax_error(const std::string& msg, int line, int col, int length = 1);
    
    // 语义错误
    void semantic_error(const std::string& msg, int line, int col, int length = 1);
    
    // 代码生成错误
    void codegen_error(const std::string& msg, int line, int col, int length = 1);
    
    // 恐慌模式：进入错误恢复状态
    void enter_panic_mode();
    
    // 退出恐慌模式
    void exit_panic_mode();
    
    bool is_in_panic_mode() const { return in_panic_mode; }
    
    // 同步：丢弃 token 直到同步点
    void synchronize();
    
    // 获取所有错误
    const std::vector<CompilerError>& get_errors() const;
    
    // 是否有错误
    bool has_errors() const;
    
    // 是否有严重错误 (超过最大数量)
    bool has_critical_errors() const;
    
    // 错误数量
    size_t error_count() const;
    
    // 清除所有错误
    void clear();
    
    // 打印所有错误
    void print_errors(std::ostream& out = std::cerr) const;
    
    // 错误类型转字符串
    static std::string error_type_to_string(ErrorType t);
    
private:
    void add_error(ErrorType type, const std::string& msg, int line, int col, int length);
};

} // namespace pascal_s
