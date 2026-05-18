#pragma once
#include <string>
#include <variant>
#include <iostream>

namespace pascal_s {

enum class TokenType {
    // 关键字
    KW_PROGRAM, KW_CONST, KW_VAR, KW_FUNCTION, KW_PROCEDURE,
    KW_BEGIN, KW_END, KW_IF, KW_THEN, KW_ELSE, KW_WHILE, KW_DO,
    KW_REPEAT, KW_UNTIL,
    KW_FOR, KW_TO, KW_DOWNTO, KW_INTEGER, KW_REAL, KW_BOOLEAN,
    KW_CHAR, KW_ARRAY, KW_OF, KW_NOT, KW_AND, KW_DIV, KW_MOD,
    KW_OR,
    
    // 内置过程/函数
    KW_WRITE, KW_WRITELN, KW_READ, KW_READLN,
    
    // 运算符
    OP_ASSIGN,     // :=
    OP_ADD, OP_SUB, OP_OR,
    OP_MUL, OP_DIV_REAL, OP_MOD, OP_AND,
    OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE,
    
    // 分隔符
    DELIM_LPAREN, DELIM_RPAREN,
    DELIM_LBRACKET, DELIM_RBRACKET,
    DELIM_SEMICOLON, DELIM_COMMA, DELIM_COLON, DELIM_DOT, DELIM_DOTDOT,
    
    // 字面量与标识符
    LIT_INTEGER, LIT_REAL, LIT_STRING, LIT_CHAR,
    TOK_IDENTIFIER,
    
    // 特殊
    TOK_EOF, TOK_ERROR
};

// Token 值 (使用 variant 存储不同类型)
using TokenValue = std::variant<
    std::monostate,      // 无值
    int,                 // 整数
    double,              // 实数
    std::string,         // 标识符/字符串
    char                 // 字符
>;

struct Token {
    TokenType type;
    TokenValue value;
    int line;
    int column;
    
    Token(TokenType t = TokenType::TOK_EOF, 
          TokenValue v = std::monostate{},
          int l = 0, int c = 0)
        : type(t), value(v), line(l), column(c) {}
    
    // 便捷访问
    int as_int() const { 
        if (auto pv = std::get_if<int>(&value)) return *pv;
        return 0; 
    }
    double as_real() const { 
        if (auto pv = std::get_if<double>(&value)) return *pv;
        return 0.0; 
    }
    const std::string& as_string() const { 
        static std::string empty;
        if (auto pv = std::get_if<std::string>(&value)) return *pv;
        return empty;
    }
    char as_char() const { 
        if (auto pv = std::get_if<char>(&value)) return *pv;
        return '\0';
    }
};

// 工具函数：Token 类型转字符串
inline std::string token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::KW_PROGRAM: return "PROGRAM";
        case TokenType::KW_CONST: return "CONST";
        case TokenType::KW_VAR: return "VAR";
        case TokenType::KW_FUNCTION: return "FUNCTION";
        case TokenType::KW_PROCEDURE: return "PROCEDURE";
        case TokenType::KW_BEGIN: return "BEGIN";
        case TokenType::KW_END: return "END";
        case TokenType::KW_IF: return "IF";
        case TokenType::KW_THEN: return "THEN";
        case TokenType::KW_ELSE: return "ELSE";
        case TokenType::KW_WHILE: return "WHILE";
        case TokenType::KW_DO: return "DO";
        case TokenType::KW_REPEAT: return "REPEAT";
        case TokenType::KW_UNTIL: return "UNTIL";
        case TokenType::KW_FOR: return "FOR";
        case TokenType::KW_TO: return "TO";
        case TokenType::KW_DOWNTO: return "DOWNTO";
        case TokenType::KW_INTEGER: return "INTEGER";
        case TokenType::KW_REAL: return "REAL";
        case TokenType::KW_BOOLEAN: return "BOOLEAN";
        case TokenType::KW_CHAR: return "CHAR";
        case TokenType::KW_ARRAY: return "ARRAY";
        case TokenType::KW_OF: return "OF";
        case TokenType::KW_NOT: return "NOT";
        case TokenType::KW_AND: return "AND";
        case TokenType::KW_DIV: return "DIV";
        case TokenType::KW_MOD: return "MOD";
        case TokenType::KW_OR: return "OR";
        case TokenType::KW_WRITE: return "WRITE";
        case TokenType::KW_WRITELN: return "WRITELN";
        case TokenType::OP_ASSIGN: return ":=";
        case TokenType::OP_ADD: return "+";
        case TokenType::OP_SUB: return "-";
        case TokenType::OP_MUL: return "*";
        case TokenType::OP_EQ: return "=";
        case TokenType::OP_NE: return "<>";
        case TokenType::OP_LT: return "<";
        case TokenType::OP_LE: return "<=";
        case TokenType::OP_GT: return ">";
        case TokenType::OP_GE: return ">=";
        case TokenType::DELIM_LPAREN: return "(";
        case TokenType::DELIM_RPAREN: return ")";
        case TokenType::DELIM_LBRACKET: return "[";
        case TokenType::DELIM_RBRACKET: return "]";
        case TokenType::DELIM_SEMICOLON: return ";";
        case TokenType::DELIM_COMMA: return ",";
        case TokenType::DELIM_COLON: return ":";
        case TokenType::DELIM_DOT: return ".";
        case TokenType::DELIM_DOTDOT: return "..";
        case TokenType::LIT_INTEGER: return "INTEGER_LITERAL";
        case TokenType::LIT_REAL: return "REAL_LITERAL";
        case TokenType::LIT_STRING: return "STRING_LITERAL";
        case TokenType::LIT_CHAR: return "CHAR_LITERAL";
        case TokenType::TOK_IDENTIFIER: return "IDENTIFIER";
        case TokenType::TOK_EOF: return "EOF";
        case TokenType::TOK_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

} // namespace pascal_s
