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
    bool recovered;
    
    CompilerError(ErrorType t, const std::string& msg, int l, int c, bool rec = false)
        : type(t), message(msg), line(l), column(c), recovered(rec) {}
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
    static ErrorHandler& instance() {
        static ErrorHandler inst;
        return inst;
    }
    
    void set_source_lines(const std::vector<std::string>& lines) {
        source_lines = lines;
    }
    
    // 词法错误
    void lexical_error(const std::string& msg, int line, int col) {
        add_error(ErrorType::LEXICAL, msg, line, col);
    }
    
    // 语法错误
    void syntax_error(const std::string& msg, int line, int col) {
        add_error(ErrorType::SYNTAX, msg, line, col);
    }
    
    // 语义错误
    void semantic_error(const std::string& msg, int line, int col) {
        add_error(ErrorType::SEMANTIC, msg, line, col);
    }
    
    // 代码生成错误
    void codegen_error(const std::string& msg, int line, int col) {
        add_error(ErrorType::CODE_GENERATION, msg, line, col);
    }
    
    // 恐慌模式：进入错误恢复状态
    void enter_panic_mode() {
        in_panic_mode = true;
    }
    
    // 退出恐慌模式
    void exit_panic_mode() {
        in_panic_mode = false;
    }
    
    bool is_in_panic_mode() const { return in_panic_mode; }
    
    // 同步：丢弃 token 直到同步点
    void synchronize() {
        // 由 parser 调用，在找到同步 token 后调用
        exit_panic_mode();
    }
    
    // 获取所有错误
    const std::vector<CompilerError>& get_errors() const {
        return errors;
    }
    
    // 是否有错误
    bool has_errors() const {
        return !errors.empty();
    }
    
    // 是否有严重错误 (超过最大数量)
    bool has_critical_errors() const {
        return errors.size() >= MAX_ERRORS;
    }
    
    // 错误数量
    size_t error_count() const {
        return errors.size();
    }
    
    // 清除所有错误
    void clear() {
        errors.clear();
        reported_positions.clear();
        in_panic_mode = false;
    }
    
    // 打印所有错误
    void print_errors(std::ostream& out = std::cerr) const {
        for (const auto& err : errors) {
            out << "[";
            switch (err.type) {
                case ErrorType::LEXICAL: out << "Lexical Error"; break;
                case ErrorType::SYNTAX: out << "Syntax Error"; break;
                case ErrorType::SEMANTIC: out << "Semantic Error"; break;
                case ErrorType::CODE_GENERATION: out << "Codegen Error"; break;
            }
            out << "] line " << err.line << ", col " << err.column 
                << ": " << err.message << std::endl;
                
            // Caret diagnostics (Clang style ~~~~^)
            if (err.line > 0 && err.line <= static_cast<int>(source_lines.size())) {
                std::string line_content = source_lines[err.line - 1];
                out << "    " << line_content << std::endl;
                if (err.column > 0) {
                    out << "    ";
                    for (int i = 0; i < err.column - 1 && i < static_cast<int>(line_content.length()); ++i) {
                        out << (line_content[i] == '\t' ? '\t' : ' ');
                    }
                    out << "^~~~~" << std::endl;
                }
            }
            out << std::endl;
        }
    }
    
    // 错误类型转字符串
    static std::string error_type_to_string(ErrorType t) {
        switch (t) {
            case ErrorType::LEXICAL: return "Lexical";
            case ErrorType::SYNTAX: return "Syntax";
            case ErrorType::SEMANTIC: return "Semantic";
            case ErrorType::CODE_GENERATION: return "Code Generation";
            default: return "Unknown";
        }
    }
    
private:
    void add_error(ErrorType type, const std::string& msg, int line, int col) {
        // 防雪崩：检查是否已报告过此位置
        auto pos = std::make_pair(line, col);
        if (reported_positions.count(pos)) return;
        
        // 检查错误数量限制
        if (errors.size() >= MAX_ERRORS) {
            errors.emplace_back(type, "Too many errors. Compilation aborted.", line, col);
            return;
        }
        
        reported_positions.insert(pos);
        errors.emplace_back(type, msg, line, col);
    }
};

} // namespace pascal_s
