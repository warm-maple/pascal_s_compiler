%{
#include <cstring>
#include <string>
#include <algorithm>
#include "ast.h"
#include "parser.tab.hpp"

// BEGIN 和 flex 内部宏冲突，需要取消定义
#ifdef BEGIN
#undef BEGIN
#endif

extern int yylineno;

// 把标识符转成小写（Pascal 不区分大小写）
static char* strdup_lower(const char* s) {
    char* r = strdup(s);
    for (char* p = r; *p; ++p) *p = tolower(*p);
    return r;
}
%}

%option noyywrap
%option yylineno

DIGIT    [0-9]
LETTER   [a-zA-Z_]
ALNUM    [a-zA-Z0-9_]

%%
 /* 空白和注释 */
[ \t\r]+  { /* 跳过空白 */ }
\n        { /* 换行 */ }
\{[^}]*\} { /* Pascal 块注释 { ... } */ }
\(\*.*?\*\) { /* 另一种块注释 (* ... *) */ }
\/\/.*    { /* 行注释 */ }

 /* Pascal-S 关键字（大小写不敏感，用字符类匹配） */
[Pp][Rr][Oo][Gg][Rr][Aa][Mm]  { yylval.sval = strdup(yytext); return PROGRAM; }
[Cc][Oo][Nn][Ss][Tt]          { yylval.sval = strdup(yytext); return CONST; }
[Vv][Aa][Rr]                  { yylval.sval = strdup(yytext); return VAR; }
[Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn] { yylval.sval = strdup(yytext); return FUNCTION; }
[Pp][Rr][Oo][Cc][Ee][Dd][Uu][Rr][Ee] { yylval.sval = strdup(yytext); return PROCEDURE; }
[Bb][Ee][Gg][Ii][Nn]          { yylval.sval = strdup(yytext); return BEGIN_KW; }
[Ee][Nn][Dd]                  { yylval.sval = strdup(yytext); return END; }
[Ii][Ff]                      { yylval.sval = strdup(yytext); return IF; }
[Tt][Hh][Ee][Nn]              { yylval.sval = strdup(yytext); return THEN; }
[Ee][Ll][Ss][Ee]              { yylval.sval = strdup(yytext); return ELSE; }
[Ww][Hh][Ii][Ll][Ee]          { yylval.sval = strdup(yytext); return WHILE; }
[Dd][Oo]                      { yylval.sval = strdup(yytext); return DO; }
[Ff][Oo][Rr]                  { yylval.sval = strdup(yytext); return FOR; }
[Tt][Oo]                      { yylval.sval = strdup(yytext); return TO; }
[Dd][Oo][Ww][Nn][Tt][Oo]      { yylval.sval = strdup(yytext); return DOWNTO; }

 /* 数据类型关键字 */
[Ii][Nn][Tt][Ee][Gg][Ee][Rr]  { yylval.sval = strdup(yytext); return INTEGER; }
[Rr][Ee][Aa][Ll]              { yylval.sval = strdup(yytext); return REAL; }
[Bb][Oo][Oo][Ll][Ee][Aa][Nn]  { yylval.sval = strdup(yytext); return BOOLEAN; }
[Cc][Hh][Aa][Rr]              { yylval.sval = strdup(yytext); return CHAR; }
[Aa][Rr][Rr][Aa][Yy]          { yylval.sval = strdup(yytext); return ARRAY; }
[Oo][Ff]                      { yylval.sval = strdup(yytext); return OF; }

 /* 逻辑和算术关键字 */
[Nn][Oo][Tt]                  { yylval.sval = strdup(yytext); return NOT; }
[Aa][Nn][Dd]                  { yylval.sval = strdup(yytext); return AND; }
[Oo][Rr]                      { yylval.sval = strdup(yytext); return OR; }
[Dd][Ii][Vv]                  { yylval.sval = strdup(yytext); return DIV; }
[Mm][Oo][Dd]                  { yylval.sval = strdup(yytext); return MOD; }
[Ww][Rr][Ii][Tt][Ee]          { yylval.sval = strdup(yytext); return WRITE; }
[Ww][Rr][Ii][Tt][Ee][Ll][Nn]  { yylval.sval = strdup(yytext); return WRITELN; }

 /* 运算符 */
":="        { yylval.sval = strdup(":="); return ASSIGN; }
"\+"        { yylval.sval = strdup("+"); return ADDOP; }
"\-"        { yylval.sval = strdup("-"); return ADDOP; }
"*"         { yylval.sval = strdup("*"); return MULOP; }
"/"         { yylval.sval = strdup("/"); return MULOP; }
"="         { yylval.sval = strdup("="); return RELOP; }
"<>"        { yylval.sval = strdup("<>"); return RELOP; }
"<="        { yylval.sval = strdup("<="); return RELOP; }
">="        { yylval.sval = strdup(">="); return RELOP; }
"<"         { yylval.sval = strdup("<"); return RELOP; }
">"         { yylval.sval = strdup(">"); return RELOP; }

 /* 界符 */
\(          { return LPAREN; }
\)          { return RPAREN; }
\[          { return LBRACKET; }
\]          { return RBRACKET; }
;           { return SEMICOLON; }
,           { return COMMA; }
:           { return COLON; }
\.          { return DOT; }
\.\.        { return DOTDOT; }

 /* 特殊标识符：true/false/read/readln 等当作普通标识符处理 */
[Tt][Rr][Uu][Ee]              { yylval.sval = strdup("true"); return IDENTIFIER; }
[Ff][Aa][Ll][Ss][Ee]          { yylval.sval = strdup("false"); return IDENTIFIER; }
[Rr][Ee][Aa][Dd]              { yylval.sval = strdup("read"); return IDENTIFIER; }
[Rr][Ee][Aa][Dd][Ll][Nn]      { yylval.sval = strdup("readln"); return IDENTIFIER; }
[Bb][Rr][Ee][Aa][Kk]          { yylval.sval = strdup("break"); return IDENTIFIER; }
[Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee] { yylval.sval = strdup("continue"); return IDENTIFIER; }

 /* 字面量 */
{DIGIT}+                { yylval.ival = atoi(yytext); return INTEGER_LITERAL; }
{DIGIT}+\.{DIGIT}+      { yylval.dval = atof(yytext); return REAL_LITERAL; }
\'[^\']\' { yylval.cval = yytext[1]; return CHAR_LITERAL; }
\'[^']*\' {
    /* 字符串字面量（多字符） */
    std::string s(yytext);
    if (s.length() >= 2) s = s.substr(1, s.length() - 2);
    yylval.sval = strdup(s.c_str());
    return STRING_LITERAL;
}

 /* 标识符（转小写存储） */
{LETTER}{ALNUM}* { yylval.sval = strdup_lower(yytext); return IDENTIFIER; }

 /* 忽略其他字符 */
. { }
%%
