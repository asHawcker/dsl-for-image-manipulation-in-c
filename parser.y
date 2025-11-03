%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

Ast *root;
int yylex();
void yyerror(const char *s) { fprintf(stderr,"Error: %s\n",s); }
%}

%union {
    char *sval;
    double fval;
    int ival;
    Ast *ast;
}

%token <sval> IDENT
%token <ival> TRUE FALSE
%token DEF RETURN IF_TK ELSE_TK FOR WHILE BREAK CONTINUE NULLVAL
%token IMAGE_TYPE INT_TYPE FLOAT_TYPE STRING_TYPE BOOL_TYPE
%token PIPE_OP
%token PLUS MINUS MUL DIV MOD
%token PLUS_ASSIGN MINUS_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN

%token INT_TK FLOAT_TK STRING_TK IMAGE_TK
%token <ival> INT_LIT
%token <fval> FLOAT_LIT
%token <sval> STR_LIT

%token LE_TK GE_TK EQ_TK

%right '='
%right PLUS_ASSIGN MINUS_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%nonassoc EQ_TK
%nonassoc '<' '>' LE_TK GE_TK
%left PLUS MINUS
%left MUL DIV MOD
%left PIPE_OP
%left ','
%nonassoc ELSE_TK
%nonassoc IFX

%type <ast> program stmt_list stmt simple_stmt expr primary_expr assignment call block expr_list expr_list_opt params_list params_list_opt
%type <ast> type declaration if_stmt

%start program

%%

program:
    stmt_list { root = $1; }
    ;

type:
    INT_TK   { $$ = make_type_node(TYPE_INT); }
  | FLOAT_TK { $$ = make_type_node(TYPE_FLOAT); }
  | STRING_TK{ $$ = make_type_node(TYPE_STRING); }
  | IMAGE_TK { $$ = make_type_node(TYPE_IMAGE); }
  ;

declaration:
    type IDENT '=' expr ';' { $$ = make_decl_node($1, $2, $4); }
  ;

stmt_list:
    stmt {
        Ast **temp = malloc(sizeof(Ast *));
        if (temp) {
            temp[0] = $1;
            $$ = make_block(temp, 1);
        } else {
            $$ = NULL;
        }
    }
  | stmt_list stmt {
        if ($1) {
            Ast **new_stmts = realloc($1->block.stmts, sizeof(Ast *) * ($1->block.n + 1));
            if (new_stmts) {
                $1->block.stmts = new_stmts;
                $1->block.stmts[$1->block.n++] = $2;
            }
        }
        $$ = $1;
    }
  ;

if_stmt:
    IF_TK '(' expr ')' block %prec IFX { $$ = make_if($3, $5); }
  | IF_TK '(' expr ')' block ELSE_TK if_stmt {
      Ast **stmts = malloc(sizeof(Ast*));
      stmts[0] = $7;
      Ast *else_block = make_block(stmts, 1);
      $$ = make_if_else($3, $5, else_block);
    }
  | IF_TK '(' expr ')' block ELSE_TK block { $$ = make_if_else($3, $5, $7); }
  ;

stmt:
    block
  | simple_stmt
  | if_stmt
  | WHILE '(' expr ')' block { $$ = make_while($3, $5); }
  | FOR '(' assignment ';' expr ';' assignment ')' block { $$ = make_for($3, $5, $7, $9); }
  | FOR '(' declaration expr ';' assignment ')' block { $$ = make_for($3, $4, $6, $8); }
  ;

simple_stmt:
      declaration { $$ = $1; }
    | assignment ';'      { $$ = $1; }
    | expr ';'            { $$ = make_expr_stmt($1); }
    | RETURN expr ';'     { $$ = make_return($2); }
    | BREAK ';'           { $$ = make_break(); }
    | CONTINUE ';'        { $$ = make_continue(); }
    | DEF IDENT '(' params_list_opt ')' block {
        char **params = $4 ? $4->arg_list.args : NULL;
        int nparams = $4 ? $4->arg_list.nargs : 0;
        $$ = make_func_def($2, params, nparams, $6);
    }
  ;

assignment:
    IDENT '=' expr { $$ = make_assign($1, $3); }
  | IDENT PLUS_ASSIGN expr  { Ast *i = make_ident($1); $$ = make_assign($1, make_binop_node(OP_ADD, i, $3)); }
  | IDENT MINUS_ASSIGN expr { Ast *i = make_ident($1); $$ = make_assign($1, make_binop_node(OP_SUB, i, $3)); }
  | IDENT MUL_ASSIGN expr   { Ast *i = make_ident($1); $$ = make_assign($1, make_binop_node(OP_MUL, i, $3)); }
  | IDENT DIV_ASSIGN expr   { Ast *i = make_ident($1); $$ = make_assign($1, make_binop_node(OP_DIV, i, $3)); }
  | IDENT MOD_ASSIGN expr   { Ast *i = make_ident($1); $$ = make_assign($1, make_binop_node(OP_MOD, i, $3)); }
  ;

expr:
      primary_expr { $$ = $1; }
    | expr PIPE_OP primary_expr { $$ = make_pipe($1, $3); }
    | expr PLUS expr  { $$ = make_binop_node(OP_ADD, $1, $3); }
    | expr MINUS expr { $$ = make_binop_node(OP_SUB, $1, $3); }
    | expr MUL expr   { $$ = make_binop_node(OP_MUL, $1, $3); }
    | expr DIV expr   { $$ = make_binop_node(OP_DIV, $1, $3); }
    | expr MOD expr   { $$ = make_binop_node(OP_MOD, $1, $3); }
    | expr '<' expr   { $$ = make_binop_node(OP_LT, $1, $3); }
    | expr '>' expr   { $$ = make_binop_node(OP_GT, $1, $3); }
    | expr LE_TK expr { $$ = make_binop_node(OP_LE, $1, $3); }
    | expr GE_TK expr { $$ = make_binop_node(OP_GE, $1, $3); }
    | expr EQ_TK expr { $$ = make_binop_node(OP_EQ, $1, $3); }
    ;

primary_expr:
      INT_LIT   { $$ = make_int_literal($1); }
    | FLOAT_LIT { $$ = make_float_literal($1); }
    | STR_LIT   { $$ = make_string_literal($1); }
    | IDENT     { $$ = make_ident($1); }
    | call      { $$ = $1; }
    | '(' expr ')' { $$ = $2; }
    ;

call:
    IDENT '(' expr_list_opt ')' {
        Ast **args = $3 ? $3->block.stmts : NULL;
        int nargs = $3 ? $3->block.n : 0;
        $$ = make_call($1, args, nargs);
    }
    ;

block:
    '{' stmt_list '}' { $$ = $2; }
    ;

expr_list:
    expr {
        Ast **temp = malloc(sizeof(Ast *));
        if (temp) {
            temp[0] = $1;
            $$ = make_block(temp, 1);
        } else {
            $$ = NULL;
        }
    }
  | expr_list ',' expr {
        if ($1) {
            Ast **new_args = realloc($1->block.stmts, sizeof(Ast *) * ($1->block.n + 1));
            if (new_args) {
                $1->block.stmts = new_args;
                $1->block.stmts[$1->block.n++] = $3;
            }
        }
        $$ = $1;
    }
  ;

expr_list_opt:
    expr_list { $$ = $1; }
  | /* empty */ { $$ = NULL; }
  ;

params_list:
    IDENT { $$ = make_arg_list($1); }
  | params_list ',' IDENT { $$ = append_arg($1, $3); }
  ;

params_list_opt:
    params_list { $$ = $1; }
  | /* empty */ { $$ = NULL; }
  ;

%%

/* No main() here; use main.c for yyparse() */
