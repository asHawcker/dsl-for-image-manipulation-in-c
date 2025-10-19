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

%token <str> IDENT STRING
%token <num> NUMBER
%token <i> TRUE FALSE
%token DEF RETURN IF ELSE FOR WHILE BREAK CONTINUE NULLVAL
%token IMAGE_TYPE INT_TYPE FLOAT_TYPE STRING_TYPE BOOL_TYPE
%token PIPE_OP
%token EQ NEQ GT LT GE LE ASSIGN PLUS MINUS MUL DIV MOD

%left PIPE_OP  /* Left-associative precedence for pipelines */
%left ','      /* Left-associative for argument lists */
%expect 1      /* Expect 1 harmless shift/reduce conflict (comma in lists) */

%type <ast> program stmt_list stmt expr primary_expr assignment call block expr_list expr_list_opt params_list params_list_opt

%start program

%%

program: 
    stmt_list { root = $1; }
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
      assignment ';'       { $$ = $1; }
    | expr ';'             { $$ = make_expr_stmt($1); }
    | RETURN expr ';'      { $$ = make_return($2); }
    | IF '(' expr ')' block ELSE block { $$ = make_if_else($3, $5, $7); }
    | IF '(' expr ')' block { $$ = make_if($3, $5); }
    | WHILE '(' expr ')' block { $$ = make_while($3, $5); }
    | FOR '(' assignment ';' expr ';' assignment ')' block { $$ = make_for($3, $5, $7, $9); }
    | BREAK ';'             { $$ = make_break(); }
    | CONTINUE ';'          { $$ = make_continue(); }
    | DEF IDENT '(' params_list_opt ')' block { 
        char **params = $4 ? $4->arg_list.args : NULL;
        int nparams = $4 ? $4->arg_list.nargs : 0;
        $$ = make_func_def($2, params, nparams, $6); 
    }
    ;

assignment:
    IDENT ASSIGN expr { $$ = make_assign($1, $3); }
    ;

expr:
    primary_expr { $$ = $1; }
    | expr PIPE_OP primary_expr { $$ = make_pipe($1, $3); }
    ;

primary_expr:
    NUMBER { $$ = make_number($1); }
    | STRING { $$ = make_string($1); }
    | IDENT { $$ = make_ident($1); }
    | call { $$ = $1; }
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
