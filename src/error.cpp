#include "error.h"
#include <algorithm>

namespace pascal_s {

ErrorHandler& ErrorHandler::instance() {
    static ErrorHandler inst;
    return inst;
}

void ErrorHandler::set_source_lines(const std::vector<std::string>& lines) {
    source_lines = lines;
}

void ErrorHandler::lexical_error(const std::string& msg, int line, int col, int length) {
    add_error(ErrorType::LEXICAL, msg, line, col, length);
}

void ErrorHandler::syntax_error(const std::string& msg, int line, int col, int length) {
    add_error(ErrorType::SYNTAX, msg, line, col, length);
}

void ErrorHandler::semantic_error(const std::string& msg, int line, int col, int length) {
    add_error(ErrorType::SEMANTIC, msg, line, col, length);
}

void ErrorHandler::codegen_error(const std::string& msg, int line, int col, int length) {
    add_error(ErrorType::CODE_GENERATION, msg, line, col, length);
}

void ErrorHandler::enter_panic_mode() {
    // `panic_mode` 用来抑制同一处语法错误触发的重复诊断；真正的 panic-mode 恢复点由 parser 中的 `error` 产生式决定。
    in_panic_mode = true;
}

void ErrorHandler::exit_panic_mode() {
    in_panic_mode = false;
}

void ErrorHandler::synchronize() {
    // 当前实现里“同步”意味着允许下一段独立语法片段重新报错，而不是在这里直接吞掉后续 token。
    exit_panic_mode();
}

const std::vector<CompilerError>& ErrorHandler::get_errors() const {
    return errors;
}

bool ErrorHandler::has_errors() const {
    return !errors.empty();
}

bool ErrorHandler::has_critical_errors() const {
    return errors.size() >= MAX_ERRORS;
}

size_t ErrorHandler::error_count() const {
    return errors.size();
}

void ErrorHandler::clear() {
    errors.clear();
    reported_positions.clear();
    in_panic_mode = false;
}

void ErrorHandler::print_errors(std::ostream& out) const {
    for (const auto& err : errors) {
        out << "[" << error_type_to_string(err.type) << " Error] line " << err.line
            << ", col " << err.column << ": " << err.message << std::endl;

        if (err.line > 0 && err.line <= static_cast<int>(source_lines.size())) {
            const std::string& line_content = source_lines[err.line - 1];
            out << "    " << line_content << std::endl;
            if (err.column > 0) {
                out << "    ";
                for (int i = 0; i < err.column - 1 && i < static_cast<int>(line_content.length()); ++i) {
                    out << (line_content[i] == '\t' ? '\t' : ' ');
                }
                out << "^";
                int wave_count = std::max(0, err.length - 1);
                for (int i = 0; i < wave_count; ++i) {
                    out << "~";
                }
                std::endl(out);
            }
        }
        out << std::endl;
    }
}

std::string ErrorHandler::error_type_to_string(ErrorType t) {
    switch (t) {
        case ErrorType::LEXICAL: return "Lexical";
        case ErrorType::SYNTAX: return "Syntax";
        case ErrorType::SEMANTIC: return "Semantic";
        case ErrorType::CODE_GENERATION: return "Codegen";
        default: return "Unknown";
    }
}

void ErrorHandler::add_error(ErrorType type, const std::string& msg, int line, int col, int length) {
    auto pos = std::make_pair(line, col);
    // 同一行列的重复错误通常来自同一处级联失败，这里直接去重，避免输出刷屏。
    if (reported_positions.count(pos)) {
        return;
    }

    if (errors.size() >= MAX_ERRORS) {
        errors.emplace_back(type, "Too many errors. Compilation aborted.", line, col, length);
        return;
    }

    reported_positions.insert(pos);
    errors.emplace_back(type, msg, line, col, std::max(length, 1));
}

} // namespace pascal_s
