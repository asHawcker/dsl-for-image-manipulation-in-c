#include "optimize.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_ast_node_wrapper(Ast *ast) {
    if (!ast) return;
    
    switch (ast->type) {
        case AST_STRING_LIT:
            free(ast->sval);
            break;
        case AST_IDENT:
            free(ast->ident.str);
            break;
        case AST_CALL:
            free(ast->call.name);
            break;
        case AST_DECL:
            free(ast->decl.name);
            break;
        case AST_FUNC_DEF:
            free(ast->func_def.name);
            for(int i=0; i<ast->func_def.nparams; i++) free(ast->func_def.params[i]);
            free(ast->func_def.params);
            break;
        case AST_ARG_LIST: 
            for(int i=0; i<ast->arg_list.nargs; i++) free(ast->arg_list.args[i]);
            free(ast->arg_list.args);
            break;
        default:
            break;
    }
    free(ast);
}

Ast *optimize_expr(Ast *expr);
Ast *optimize_stmt(Ast *stmt);
Ast *optimize_block(Ast *block);

Ast *inline_pipe_and_assignment(Ast *expr) {
    if (!expr) return NULL;

    if (expr->type == AST_BINOP) {
        expr->binop.left = optimize_expr(expr->binop.left);
        expr->binop.right = optimize_expr(expr->binop.right);
        return expr; 
    }
    if (expr->type == AST_CALL) {
        for (int i = 0; i < expr->call.nargs; i++) {
            expr->call.args[i] = optimize_expr(expr->call.args[i]);
        }
        return expr; 
    }

    if (expr->type == AST_PIPELINE) {
        Ast *left_opt = inline_pipe_and_assignment(expr->pipe.left); 
        Ast *right_call = expr->pipe.right;

        if (right_call->type != AST_CALL) {
            free_ast_node_wrapper(expr);
            return left_opt; 
        }
        
        int new_nargs = right_call->call.nargs + 1;
        Ast **new_args = malloc(sizeof(Ast*) * new_nargs);
        if (!new_args) { free_ast_node_wrapper(expr); return left_opt; }

        new_args[0] = left_opt;

        for (int i = 0; i < right_call->call.nargs; i++) {
            new_args[i + 1] = optimize_expr(right_call->call.args[i]); 
        }

        Ast *combined_call = make_call(right_call->call.name, new_args, new_nargs);

        free(right_call->call.args); 

        free_ast_node_wrapper(right_call); 

        free_ast_node_wrapper(expr); 

        return combined_call;
    }
    return expr;
}

Ast *optimize_expr(Ast *expr) {
    if (!expr) return NULL;
    return inline_pipe_and_assignment(expr);
}

Ast *optimize_decl(Ast *decl) {
    if (!decl) return NULL;
    decl->decl.expr = optimize_expr(decl->decl.expr);
    return decl;
}

Ast *optimize_assign(Ast *assign) {
    if (!assign) return NULL;
    assign->assign.expr = optimize_expr(assign->assign.expr);
    return assign;
}

Ast *optimize_expr_stmt(Ast *expr_stmt) {
    if (!expr_stmt) return NULL;
    expr_stmt->expr_stmt.expr = optimize_expr(expr_stmt->expr_stmt.expr);
    return expr_stmt;
}

Ast *optimize_stmt(Ast *stmt) {
    if (!stmt) return NULL;

    switch (stmt->type) {
        case AST_DECL: 
            return optimize_decl(stmt);
        case AST_ASSIGN: 
            return optimize_assign(stmt);
        case AST_EXPR_STMT:
            return optimize_expr_stmt(stmt);
        case AST_BLOCK:
            return optimize_block(stmt); 
        case AST_IF:
            stmt->if_stmt.cond = optimize_expr(stmt->if_stmt.cond);
            stmt->if_stmt.block = optimize_block(stmt->if_stmt.block);
            return stmt;
        default:
            return stmt;
    }
}

Ast *optimize_block(Ast *block) {
    if (!block || block->type != AST_BLOCK) return block;

    for (int i = 0; i < block->block.n; i++) {
        block->block.stmts[i] = optimize_stmt(block->block.stmts[i]);
    }
    return block;
}

Ast *optimize_ast(Ast *root) {
    if (!root) return NULL;
    return optimize_block(root);
}