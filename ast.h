#ifndef AST_H
#define AST_H

typedef enum { 
    TYPE_INT, 
    TYPE_FLOAT, 
    TYPE_STRING, 
    TYPE_IMAGE, 
    TYPE_UNKNOWN 
} TypeId;

typedef enum {
    AST_ASSIGN,
    AST_EXPR_STMT,
    AST_CALL,
    AST_PIPELINE,
    AST_BLOCK,
    AST_RETURN,
    AST_IF,
    AST_IF_ELSE,
    AST_WHILE,
    AST_FOR,
    AST_BREAK,
    AST_CONTINUE,
    AST_FUNC_DEF,
    AST_ARG_LIST,
    AST_NUMBER,
    AST_STRING,
    AST_IDENT,
    AST_INT_LIT, 
    AST_FLOAT_LIT, 
    AST_STRING_LIT, 
    AST_NULL_LIT,
    AST_DECL, 
    AST_TYPE,
    AST_BINOP,
} AstType;

typedef struct Ast {
    AstType type;
    TypeId type2;
    union {
        int ival;
        double fval;
        char *sval;
        struct { struct Ast *type_node; char *name; struct Ast *expr; } decl;
        struct { char *name; struct Ast *expr; } assign;
        struct { struct Ast *expr; } expr_stmt;
        struct { char *name; struct Ast **args; int nargs; } call;
        struct { struct Ast *left; struct Ast *right; } pipe;
        struct { struct Ast **stmts; int n; } block;
        struct { struct Ast *expr; } ret;
        struct { struct Ast *cond; struct Ast *block; } if_stmt;
        struct { struct Ast *cond; struct Ast *then_block; struct Ast *else_block; } if_else_stmt;
        struct { struct Ast *cond; struct Ast *block; } while_stmt;
        struct { struct Ast *init; struct Ast *cond; struct Ast *update; struct Ast *block; } for_stmt;
        struct { char **args; int nargs; } arg_list;
        struct { char *name; char **params; int nparams; struct Ast *body; } func_def;
        struct { double num; } number;
        struct { char *str; } string;
        struct { char *str; } ident;
        struct { struct Ast *left; int op; struct Ast *right; } binop;
    };
} Ast;

Ast *make_int_literal(int v);
Ast *make_float_literal(double v);
Ast *make_string_literal(const char *s);
Ast *make_type_node(TypeId t);
Ast *make_decl_node(Ast *type_node, char *name, Ast *expr);

// Constructors
Ast *make_assign(char *name, Ast *expr);
Ast *make_expr_stmt(Ast *expr);
Ast *make_call(char *name, Ast **args, int nargs);
Ast *make_pipe(Ast *left, Ast *right);
Ast *make_block(Ast **stmts, int n);
Ast *make_return(Ast *expr);
Ast *make_if(Ast *cond, Ast *block);
Ast *make_if_else(Ast *cond, Ast *then_block, Ast *else_block);
Ast *make_while(Ast *cond, Ast *block);
Ast *make_for(Ast *init, Ast *cond, Ast *update, Ast *block);
Ast *make_break();
Ast *make_continue();
Ast *make_func_def(char *name, char **params, int nparams, Ast *body);
Ast *make_arg_list(char *name);
Ast *append_arg(Ast *list, char *name);
Ast *make_number(double val);
Ast *make_string(char *s);
Ast *make_ident(char *name);
Ast *clone_ast(Ast *ast);
Ast *make_binop(struct Ast *left, int op, struct Ast *right);
Ast *make_null_literal();

// Utility
void free_ast(Ast *ast);
void dump_ast(Ast *ast, int indent);

#endif
