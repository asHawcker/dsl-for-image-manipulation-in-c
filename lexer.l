%{
#include "ast.h"
#include "parser.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
%}

%option noinput
%option nounput

%%

"def" { return DEF; }
"return" { return RETURN; }
"if" { return IF_TK; }
"else" { return ELSE_TK; }
"for" { return FOR; }
"while" { return WHILE; }
"break" { return BREAK; }
"continue" { return CONTINUE; }

"true" { yylval.ival = 1; return TRUE; }
"false" { yylval.ival = 0; return FALSE; }
"null" { return NULLVAL; }

"load" { yylval.sval = strdup(yytext); return IDENT; }
"save" { yylval.sval = strdup(yytext); return IDENT; }
"crop" { yylval.sval = strdup(yytext); return IDENT; }
"print" { yylval.sval = strdup(yytext); return IDENT; }
"resize" { yylval.sval = strdup(yytext); return IDENT; }
"scale" { yylval.sval = strdup(yytext); return IDENT; }
"rotate" { yylval.sval = strdup(yytext); return IDENT; }
"flipX" { yylval.sval = strdup(yytext); return IDENT; }
"flipY" { yylval.sval = strdup(yytext); return IDENT; }
"mirror" { yylval.sval = strdup(yytext); return IDENT; }
"blur" { yylval.sval = strdup(yytext); return IDENT; }
"sharpen" { yylval.sval = strdup(yytext); return IDENT; }
"grayscale" { yylval.sval = strdup(yytext); return IDENT; }
"invert" { yylval.sval = strdup(yytext); return IDENT; }
"brighten" { yylval.sval = strdup(yytext); return IDENT; }
"contrast" { yylval.sval = strdup(yytext); return IDENT; }
"threshold" { yylval.sval = strdup(yytext); return IDENT; }
"cannyedge" { yylval.sval = strdup(yytext); return IDENT; }
"blend" { yylval.sval = strdup(yytext); return IDENT; }
"composite" { yylval.sval = strdup(yytext); return IDENT; }
"convolve" { yylval.sval = strdup(yytext); return IDENT; }

"image" { return IMAGE_TK; }
"int" { return INT_TK; }
"float" { return FLOAT_TK; }
"string" { return STRING_TK; }
"bool" { return BOOL_TYPE; }


"<=" { return LE_TK; }
">=" { return GE_TK; }
"==" { return EQ_TK; }

"{" { return '{'; }
"}" { return '}'; }


"<" { return '<'; }
">" { return '>'; }
"=" { return '='; }

"|>" { return PIPE_OP; }
"+=" { return PLUS_ASSIGN; }
"-=" { return MINUS_ASSIGN; }
"*=" { return MUL_ASSIGN; }
"/=" { return DIV_ASSIGN; }
"%=" { return MOD_ASSIGN; }
"+" { return PLUS; }
"-" { return MINUS; }
"*" { return MUL; }
"/" { return DIV; }
"%" { return MOD; }


[0-9]+"."[0-9]*([eE][+-]?[0-9]+)? { yylval.fval = atof(yytext); return FLOAT_LIT; }
[0-9]+ { yylval.ival = atoi(yytext); return INT_LIT; }
\"([^\\\"]|\\.)*\" { yylval.sval = strdup(yytext+1); yylval.sval[strlen(yylval.sval)-1] = '\0'; return STR_LIT; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.sval = strdup(yytext); return IDENT; }


[ \t\n\r]+ { }
";" { return ';'; }
"," { return ','; }
"(" { return '('; }
")" { return ')'; }

. { fprintf(stderr,"Unknown char: %s\n", yytext); exit(1); }

%%

int yywrap(void) { return 1; }
