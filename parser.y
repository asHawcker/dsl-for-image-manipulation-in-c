%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

Ast *root;
int yylex();
void yyerror(const char *s) { fprintf(stderr,"Error: %s\n",s); }
%}

%union {
    char *str;
    double num;
    int i;
    Ast *ast;
}

/* --- TOKEN DECLARATIONS (CLEANED) --- */

/* Kept <str> IDENT, removed obsolete STRING */
%token <str> IDENT 
/* Removed obsolete NUMBER token */
%token <i> TRUE FALSE
%token DEF RETURN IF ELSE FOR WHILE BREAK CONTINUE NULLVAL
%token IMAGE_TYPE INT_TYPE FLOAT_TYPE STRING_TYPE BOOL_TYPE
%token PIPE_OP
%token EQ NEQ GT LT GE LE ASSIGN PLUS MINUS MUL DIV MOD

/* Type keywords (from your example) */
%token INT_TK FLOAT_TK STRING_TK IMAGE_TK

/* New typed literal tokens (from your example) */
%token <i> INT_LIT
%token <num> FLOAT_LIT
%token <str> STR_LIT

/* --- END TOKEN DECLARATIONS --- */
%left PLUS MINUS
%left MUL DIV MOD
%left GT LT GE LE
%left EQ NEQ

%left PIPE_OP  /* Left-associative precedence for pipelines */
%left ','      /* Left-associative for argument lists */
%expect 1      /* Expect 1 harmless shift/reduce conflict (comma in lists) */

/* Merged and cleaned %type declarations */
%type <ast> program stmt_list stmt expr primary_expr assignment call block expr_list expr_list_opt params_list params_list_opt
%type <ast> type declaration 

%start program

%%

program: 
    stmt_list { root = $1; }
    ;

/* This 'type' rule was already present in your file, as requested */
type:
    INT_TK   { $$ = make_type_node(TYPE_INT); }
  | FLOAT_TK { $$ = make_type_node(TYPE_FLOAT); }
  | STRING_TK{ $$ = make_type_node(TYPE_STRING); }
  | IMAGE_TK { $$ = make_type_node(TYPE_IMAGE); }
  ;
  
declaration:
    type IDENT ASSIGN expr { 
        $$ = make_decl_node($1, $2, $4);
    }
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
            } else {
                $$ = $1;
            }
        }
        $$ = $1; 
    }
    ;

stmt:
      declaration ';' { $$ = $1; } 
    | assignment ';'      { $$ = $1; }
    | expr ';'            { $$ = make_expr_stmt($1); }
    | RETURN expr ';'     { $$ = make_return($2); }
    | IF '(' expr ')' block ELSE block { $$ = make_if_else($3, $5, $7); }
    | IF '(' expr ')' block { $$ = make_if($3, $5); }
    | WHILE '(' expr ')' block { $$ = make_while($3, $5); }
    | FOR '(' declaration ';' expr ';' assignment ')' block { $$ = make_for($3, $5, $7, $9); }
    | FOR '(' assignment ';' expr ';' assignment ')' block { $$ = make_for($3, $5, $7, $9); }
    | BREAK ';'           { $$ = make_break(); }
    | CONTINUE ';'        { $$ = make_continue(); }
    | DEF IDENT '(' params_list_opt ')' block { 
        char **params = $4 ? $4->arg_list.args : NULL;
        int nparams = $4 ? $4->arg_list.nargs : 0;
        $$ = make_func_def($2, params, nparams, $6); 
    }
    ;

assignment:
    IDENT ASSIGN expr { $$ = make_assign($1, $3); }
    ;

/* --- EXPRESSION RULES (CLEANED) --- */

/* 'expr' is for operators (like pipe) */
expr:
      primary_expr { $$ = $1; }
    | expr PLUS expr    { $$ = make_binop($1, PLUS, $3); }
    | expr MINUS expr   { $$ = make_binop($1, MINUS, $3); }
    | expr MUL expr     { $$ = make_binop($1, MUL, $3); }
    | expr DIV expr     { $$ = make_binop($1, DIV, $3); }
    | expr MOD expr     { $$ = make_binop($1, MOD, $3); }
    | expr EQ expr      { $$ = make_binop($1, EQ, $3); }
    | expr NEQ expr     { $$ = make_binop($1, NEQ, $3); }
    | expr GT expr      { $$ = make_binop($1, GT, $3); }
    | expr LT expr      { $$ = make_binop($1, LT, $3); }
    | expr GE expr      { $$ = make_binop($1, GE, $3); }
    | expr LE expr      { $$ = make_binop($1, LE, $3); }
    | expr PIPE_OP primary_expr { $$ = make_pipe($1, $3); }
    ;

/* 'primary_expr' is for terminals and calls */
primary_expr:
      INT_LIT   { $$ = make_int_literal($1); }
    | FLOAT_LIT { $$ = make_float_literal($1); }
    | STR_LIT   { $$ = make_string_literal($1); }
    | IDENT     { $$ = make_ident($1); }
    | call      { $$ = $1; }
    | '(' expr ')' { $$ = $2; }
    | TRUE      { $$ = make_int_literal($1); }
    | FALSE     { $$ = make_int_literal($1); }
    | NULLVAL   { $$ = make_null_literal(); }
    ;

/* --- END EXPRESSION RULES --- */


call:
    IDENT '(' expr_list_opt ')' { 
        Ast **args = $3 ? $3->block.stmts : NULL;
        int nargs = $3 ? $3->block.n : 0;
        $$ = make_call($1, args, nargs); 
    }
    ;

block:
    '{' stmt_list '}' { $$ = $2; }
    | stmt { 
        Ast **temp = malloc(sizeof(Ast *)); 
        if (temp) { 
            temp[0] = $1; 
            $$ = make_block(temp, 1); 
        } else { 
            $$ = NULL; 
        }
    }
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
            } else {
                $$ = $1;  // Keep on failure
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
