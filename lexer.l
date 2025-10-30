%{
#include "ast.h"
#include "parser.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
%}

%option noinput
%option nounput

ID [a-zA-Z_][a-zA-Z0-9_]*

%%

"def" { return DEF; }
"return" { return RETURN; }
"if" { return IF; }
"else" { return ELSE; }
"for" { return FOR; }
"while" { return WHILE; }
"break" { return BREAK; }
"continue" { return CONTINUE; }

"true" { yylval.i = 1; return TRUE; }
"false" { yylval.i = 0; return FALSE; }
"null" { return NULLVAL; }

"load" { yylval.str = strdup(yytext); return IDENT; }
"save" { yylval.str = strdup(yytext); return IDENT; }
"crop" { yylval.str = strdup(yytext); return IDENT; }
"resize" { yylval.str = strdup(yytext); return IDENT; }
"scale" { yylval.str = strdup(yytext); return IDENT; }
"rotate" { yylval.str = strdup(yytext); return IDENT; }
"flipX" { yylval.str = strdup(yytext); return IDENT; }
"flipY" { yylval.str = strdup(yytext); return IDENT; }
"mirror" { yylval.str = strdup(yytext); return IDENT; }
"blur" { yylval.str = strdup(yytext); return IDENT; }
"sharpen" { yylval.str = strdup(yytext); return IDENT; }
"grayscale" { yylval.str = strdup(yytext); return IDENT; }
"invert" { yylval.str = strdup(yytext); return IDENT; }
"brighten" { yylval.str = strdup(yytext); return IDENT; }
"contrast" { yylval.str = strdup(yytext); return IDENT; }
"threshold" { yylval.str = strdup(yytext); return IDENT; }
"cannyedge" { yylval.str = strdup(yytext); return IDENT; }
"blend" { yylval.str = strdup(yytext); return IDENT; }
"composite" { yylval.str = strdup(yytext); return IDENT; }
"convolve" { yylval.str = strdup(yytext); return IDENT; }

"image" { return IMAGE_TK; }
"int" { return INT_TK; }
"float" { return FLOAT_TK; }
"string" { return STRING_TK; }
"bool" { return BOOL_TYPE; }

"|>" { return PIPE_OP; }
"==" { return EQ; }
"!=" { return NEQ; }
">=" { return GE; }
"<=" { return LE; }
">" { return GT; }
"<" { return LT; }
"=" { return ASSIGN; }
"+" { return PLUS; }
"-" { return MINUS; }
"*" { return MUL; }
"/" { return DIV; }
"%" { return MOD; }

[0-9]+"."[0-9]*([eE][+-]?[0-9]+)? { yylval.num = atof(yytext); return FLOAT_LIT; }
[0-9]+ { yylval.i = atoi(yytext); return INT_LIT; }
\"([^\\\"]|\\.)*\" { yylval.str = strdup(yytext+1); yylval.str[strlen(yylval.str)-1] = '\0'; return STR_LIT; }

{ID} { yylval.str = strdup(yytext); return IDENT; }


[ \t\n\r]+ {  }
";" { return ';'; }
"," { return ','; }
"(" { return '('; }
")" { return ')'; }
"{" { return '{'; }
"}" { return '}'; }

. { fprintf(stderr,"Unknown char: %s\n", yytext); exit(1); }

%%

int yywrap(void) { return 1; }
