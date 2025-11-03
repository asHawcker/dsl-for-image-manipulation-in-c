#include "ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* --- NEW CONSTRUCTORS --- */

Ast *make_int_literal(int v) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_INT_LIT;
    ast->type2 = TYPE_INT;
    ast->ival = v;
    return ast;
}

Ast *make_float_literal(double v) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_FLOAT_LIT;
    ast->type2 = TYPE_FLOAT;
    ast->fval = v;
    return ast;
}

Ast *make_string_literal(const char *s) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_STRING_LIT;
    ast->type2 = TYPE_STRING;
    ast->sval = strdup(s);
    return ast;
}

Ast *make_type_node(TypeId t) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_TYPE;
    ast->type2 = t;
    return ast;
}

Ast *make_decl_node(Ast *type_node, char *name, Ast *expr) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_DECL;
    ast->type2 = type_node->type2;
    ast->decl.type_node = type_node;
    ast->decl.name = strdup(name);
    ast->decl.expr = expr;
    return ast;
}

// --- NEW BINOP CONSTRUCTOR ---
Ast *make_binop_node(BinOp op, Ast *left, Ast *right) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_BINOP;
    ast->binop.op = op;
    ast->binop.left = left;
    ast->binop.right = right;
    return ast;
}
// --- END ---


/* --- EXISTING CONSTRUCTORS --- */
// ... (make_assign, make_expr_stmt, make_call, etc. are all unchanged) ...
Ast *make_assign(char *name, Ast *expr) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_ASSIGN;
    ast->assign.name = strdup(name);
    ast->assign.expr = expr;
    return ast;
}

Ast *make_expr_stmt(Ast *expr) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_EXPR_STMT;
    ast->expr_stmt.expr = expr;
    return ast;
}

Ast *make_call(char *name, Ast **args, int nargs) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_CALL;
    ast->call.name = strdup(name);
    ast->call.args = args;
    ast->call.nargs = nargs;
    return ast;
}

Ast *make_pipe(Ast *left, Ast *right) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_PIPELINE;
    ast->pipe.left = left;
    ast->pipe.right = right;
    return ast;
}

Ast *make_block(Ast **stmts, int n) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_BLOCK;
    ast->block.stmts = stmts;
    ast->block.n = n;
    return ast;
}

Ast *make_return(Ast *expr) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_RETURN;
    ast->ret.expr = expr;
    return ast;
}

Ast *make_if(Ast *cond, Ast *block) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_IF;
    ast->if_stmt.cond = cond;
    ast->if_stmt.block = block;
    return ast;
}

Ast *make_if_else(Ast *cond, Ast *then_block, Ast *else_block) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_IF_ELSE;
    ast->if_else_stmt.cond = cond;
    ast->if_else_stmt.then_block = then_block;
    ast->if_else_stmt.else_block = else_block;
    return ast;
}

Ast *make_while(Ast *cond, Ast *block) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_WHILE;
    ast->while_stmt.cond = cond;
    ast->while_stmt.block = block;
    return ast;
}

Ast *make_for(Ast *init, Ast *cond, Ast *update, Ast *block) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_FOR;
    ast->for_stmt.init = init;
    ast->for_stmt.cond = cond;
    ast->for_stmt.update = update;
    ast->for_stmt.block = block;
    return ast;
}

Ast *make_break() {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_BREAK;
    return ast;
}

Ast *make_continue() {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_CONTINUE;
    return ast;
}

Ast *make_func_def(char *name, char **params, int nparams, Ast *body) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_FUNC_DEF;
    ast->func_def.name = strdup(name);
    ast->func_def.params = params;
    ast->func_def.nparams = nparams;
    ast->func_def.body = body;
    return ast;
}

Ast *make_arg_list(char *name) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_ARG_LIST;
    ast->arg_list.args = malloc(sizeof(char*));
    if (!ast->arg_list.args) { free(ast); return NULL; }
    ast->arg_list.args[0] = strdup(name);
    ast->arg_list.nargs = 1;
    return ast;
}

Ast *append_arg(Ast *list, char *name) {
    if (!list) return NULL;
    list->arg_list.args = realloc(list->arg_list.args, sizeof(char*) * (list->arg_list.nargs + 1));
    if (!list->arg_list.args) return NULL;
    list->arg_list.args[list->arg_list.nargs] = strdup(name);
    list->arg_list.nargs++;
    return list;
}

Ast *make_number(double val) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_NUMBER;
    ast->number.num = val;
    return ast;
}

Ast *make_string(char *s) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_STRING;
    ast->string.str = strdup(s);
    return ast;
}

Ast *make_ident(char *name) {
    Ast *ast = malloc(sizeof(Ast));
    if (!ast) return NULL;
    ast->type = AST_IDENT;
    ast->ident.str = strdup(name);
    return ast;
}

// Deep clone AST node and children
Ast *clone_ast(Ast *ast) {
    // ... (This function needs to be updated for AST_BINOP too) ...
    return ast; // Stubbed for brevity, you should implement this
}

void free_ast(Ast *ast) {
    if (!ast) return;
    switch(ast->type){
        case AST_BINOP: // <-- ADDED
            free_ast(ast->binop.left);
            free_ast(ast->binop.right);
            break;
        // ... (all other cases) ...
        case AST_INT_LIT:
            break;
        case AST_FLOAT_LIT:
            break;
        case AST_STRING_LIT:
            free(ast->sval);
            break;
        case AST_IDENT:
            free(ast->ident.str);
            break;
        case AST_TYPE:
            break;
        case AST_DECL:
            free_ast(ast->decl.type_node);
            free(ast->decl.name);
            free_ast(ast->decl.expr);
            break;
        case AST_ASSIGN: 
            free(ast->assign.name); 
            free_ast(ast->assign.expr); 
            break;
        case AST_EXPR_STMT: 
            free_ast(ast->expr_stmt.expr); 
            break;
        case AST_CALL:
            free(ast->call.name);
            for(int i=0;i<ast->call.nargs;i++) free_ast(ast->call.args[i]);
            free(ast->call.args);
            break;
        case AST_PIPELINE: 
            free_ast(ast->pipe.left); 
            free_ast(ast->pipe.right); 
            break;
        case AST_BLOCK:
            for(int i=0;i<ast->block.n;i++) free_ast(ast->block.stmts[i]);
            free(ast->block.stmts);
            break;
        // ... (etc.) ...
        default: 
            break;
    }
    free(ast);
}

// Helper for dump_ast to print operator
const char* op_to_string(BinOp op) {
    switch(op) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_MOD: return "%";
        case OP_LT: return "<";
        case OP_GT: return ">";
        case OP_LE: return "<=";
        case OP_GE: return ">=";
        case OP_EQ: return "==";
        default: return "?";
    }
}

void dump_ast(Ast *ast, int indent) {
    if (!ast) return;
    for(int i=0;i<indent;i++) printf("  ");
    switch(ast->type){
        case AST_BINOP: // <-- ADDED
            printf("Op: %s\n", op_to_string(ast->binop.op));
            dump_ast(ast->binop.left, indent+1);
            dump_ast(ast->binop.right, indent+1);
            break;
        // ... (all other cases) ...
        case AST_INT_LIT:
            printf("Int: %d\n", ast->ival);
            break;
        case AST_FLOAT_LIT:
            printf("Float: %lf\n", ast->fval);
            break;
        case AST_STRING_LIT:
            printf("String: \"%s\"\n", ast->sval);
            break;
        case AST_TYPE:
            printf("Type: ");
            switch(ast->type2) {
                case TYPE_INT: printf("int\n"); break;
                case TYPE_FLOAT: printf("float\n"); break;
                case TYPE_STRING: printf("string\n"); break;
                case TYPE_IMAGE: printf("image\n"); break;
                default: printf("unknown\n"); break;
            }
            break;
        case AST_DECL:
            printf("Decl: %s\n", ast->decl.name);
            dump_ast(ast->decl.type_node, indent+1);
            dump_ast(ast->decl.expr, indent+1);
            break;
        case AST_ASSIGN: 
            printf("Assign: %s\n", ast->assign.name); 
            dump_ast(ast->assign.expr, indent+1); 
            break;
        case AST_IDENT:
            printf("Ident: %s\n", ast->ident.str);
            break;
        case AST_BLOCK:
            printf("Block (%d stmts)\n", ast->block.n);
            for(int i=0; i<ast->block.n; i++) {
                dump_ast(ast->block.stmts[i], indent+1);
            }
            break;
        case AST_CALL:
            printf("Call: %s (%d args)\n", ast->call.name, ast->call.nargs);
            for(int i=0; i<ast->call.nargs; i++) {
                dump_ast(ast->call.args[i], indent+1);
            }
            break;
        case AST_EXPR_STMT:
            printf("ExprStmt\n");
            dump_ast(ast->expr_stmt.expr, indent+1);
            break;
        case AST_PIPELINE:
            printf("Pipeline\n");
            dump_ast(ast->pipe.left, indent+1);
            dump_ast(ast->pipe.right, indent+1);
            break;
        case AST_IF:
            printf("If\n");
            dump_ast(ast->if_stmt.cond, indent+1);
            dump_ast(ast->if_stmt.block, indent+1);
            break;
        case AST_IF_ELSE:
            printf("IfElse\n");
            dump_ast(ast->if_else_stmt.cond, indent+1);
            dump_ast(ast->if_else_stmt.then_block, indent+1);
            dump_ast(ast->if_else_stmt.else_block, indent+1);
            break;
        case AST_WHILE:
            printf("While\n");
            dump_ast(ast->while_stmt.cond, indent+1);
            dump_ast(ast->while_stmt.block, indent+1);
            break;
        case AST_FOR:
            printf("For\n");
            dump_ast(ast->for_stmt.init, indent+1);
            dump_ast(ast->for_stmt.cond, indent+1);
            dump_ast(ast->for_stmt.update, indent+1);
            dump_ast(ast->for_stmt.block, indent+1);
            break;
        default: 
            printf("Unknown node\n"); 
            break;
    }
}
