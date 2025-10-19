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
NUMBER [0-9]+(\.[0-9]+)?

%%

"def"            { return DEF; }
"return"         { return RETURN; }
"if"             { return IF; }
"else"           { return ELSE; }
"for"            { return FOR; }
"while"          { return WHILE; }
"break"          { return BREAK; }
"continue"       { return CONTINUE; }

"true"           { yylval.i = 1; return TRUE; }
"false"          { yylval.i = 0; return FALSE; }
"null"           { return NULLVAL; }

"load"           { yylval.str = strdup(yytext); return IDENT; }
"save"           { yylval.str = strdup(yytext); return IDENT; }
"crop"           { yylval.str = strdup(yytext); return IDENT; }
"resize"         { yylval.str = strdup(yytext); return IDENT; }
"scale"          { yylval.str = strdup(yytext); return IDENT; }
"rotate"         { yylval.str = strdup(yytext); return IDENT; }
"flip"           { yylval.str = strdup(yytext); return IDENT; }
"mirror"         { yylval.str = strdup(yytext); return IDENT; }
"blur"           { yylval.str = strdup(yytext); return IDENT; }
"sharpen"        { yylval.str = strdup(yytext); return IDENT; }
"grayscale"      { yylval.str = strdup(yytext); return IDENT; }
"invert"         { yylval.str = strdup(yytext); return IDENT; }
"brightness"     { yylval.str = strdup(yytext); return IDENT; }
"contrast"       { yylval.str = strdup(yytext); return IDENT; }
"threshold"      { yylval.str = strdup(yytext); return IDENT; }
"edge"           { yylval.str = strdup(yytext); return IDENT; }
"blend"          { yylval.str = strdup(yytext); return IDENT; }
"composite"      { yylval.str = strdup(yytext); return IDENT; }
"convolve"       { yylval.str = strdup(yytext); return IDENT; }

"image"          { return IMAGE_TYPE; }
"int"            { return INT_TYPE; }
"float"          { return FLOAT_TYPE; }
"string"         { return STRING_TYPE; }
"bool"           { return BOOL_TYPE; }

"|>"             { return PIPE_OP; }
"=="             { return EQ; }
"!="             { return NEQ; }
">="             { return GE; }
"<="             { return LE; }
">"              { return GT; }
"<"              { return LT; }
"="              { return ASSIGN; }
"+"              { return PLUS; }
"-"              { return MINUS; }
"*"              { return MUL; }
"/"              { return DIV; }
"%"              { return MOD; }

{ID}             { yylval.str = strdup(yytext); return IDENT; }
{NUMBER}         { yylval.num = atof(yytext); return NUMBER; }
\"[^\"]*\"       { char *s = strdup(yytext+1); s[strlen(s)-1] = 0; yylval.str = s; return STRING; }

[ \t\n\r]+       { /* skip whitespace */ }
";"              { return ';'; }
","              { return ','; }
"("              { return '('; }
")"              { return ')'; }
"{"              { return '{'; }
"}"              { return '}'; }

.                { fprintf(stderr,"Unknown char: %s\n", yytext); exit(1); }

%%

int yywrap(void) { return 1; }
