%{
#include <cstring>
#include <string>
#include "ast.h"
#include "parser.tab.hpp"
// 解决 BEGIN 宏冲突
#ifdef BEGIN
#undef BEGIN
#endif
extern int yylineno;
%}

%option noyywrap
%option yylineno

DIGIT    [0-9]
LETTER   [a-zA-Z_]
ALNUM    [a-zA-Z0-9_]

%%
[ \t\r]+  { }
\n        { }
\{[^}]*\} { }
\(\*.*?\*\) { }
\/\/.*    { }

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
[Ii][Nn][Tt][Ee][Gg][Ee][Rr]  { yylval.sval = strdup(yytext); return INTEGER; }
[Rr][Ee][Aa][Ll]              { yylval.sval = strdup(yytext); return REAL; }
[Bb][Oo][Oo][Ll][Ee][Aa][Nn]  { yylval.sval = strdup(yytext); return BOOLEAN; }
[Cc][Hh][Aa][Rr]              { yylval.sval = strdup(yytext); return CHAR; }
[Aa][Rr][Rr][Aa][Yy]          { yylval.sval = strdup(yytext); return ARRAY; }
[Oo][Ff]                      { yylval.sval = strdup(yytext); return OF; }
[Nn][Oo][Tt]                  { yylval.sval = strdup(yytext); return NOT; }
[Aa][Nn][Dd]                  { yylval.sval = strdup(yytext); return AND; }
[Oo][Rr]                      { yylval.sval = strdup(yytext); return OR; }
[Dd][Ii][Vv]                  { yylval.sval = strdup(yytext); return DIV; }
[Mm][Oo][Dd]                  { yylval.sval = strdup(yytext); return MOD; }
[Ww][Rr][Ii][Tt][Ee]          { yylval.sval = strdup(yytext); return WRITE; }
[Ww][Rr][Ii][Tt][Ee][Ll][Nn]  { yylval.sval = strdup(yytext); return WRITELN; }

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

\(          { return LPAREN; }
\)          { return RPAREN; }
\[          { return LBRACKET; }
\]          { return RBRACKET; }
;           { return SEMICOLON; }
,           { return COMMA; }
:           { return COLON; }
\.          { return DOT; }
\.\.        { return DOTDOT; }

{DIGIT}+ { yylval.ival = atoi(yytext); return INTEGER_LITERAL; }
{DIGIT}+\.{DIGIT}+ { yylval.dval = atof(yytext); return REAL_LITERAL; }
\'[^\']\' { yylval.cval = yytext[1]; return CHAR_LITERAL; }
\'[^']*\' {
    std::string s(yytext);
    if (s.length() >= 2) s = s.substr(1, s.length() - 2);
    yylval.sval = strdup(s.c_str());
    return STRING_LITERAL;
}
{LETTER}{ALNUM}* { yylval.sval = strdup(yytext); return IDENTIFIER; }
. { }
%%
