#include "optimize.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Memory Cleanup Utility ---
// This function frees the memory allocated for the structure itself,
// but NOT its child pointers, which are either null or moved/reused.
void free_ast_node_wrapper(Ast *ast) {
    if (!ast) return;
    
    // Manually free any direct heap-allocated members of the node.
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
            // Params array content: Must be freed here as it was strdup-ed
            for(int i=0; i<ast->func_def.nparams; i++) free(ast->func_def.params[i]);
            free(ast->func_def.params);
            break;
        case AST_ARG_LIST: 
            for(int i=0; i<ast->arg_list.nargs; i++) free(ast->arg_list.args[i]);
            free(ast->arg_list.args);
            break;
        default:
            // No direct heap-allocated members for literals, binops, etc.
            break;
    }
    // Free the struct itself
    free(ast);
}

// --- Optimization Core ---

// Forward declarations
Ast *optimize_expr(Ast *expr);
Ast *optimize_stmt(Ast *stmt);
Ast *optimize_block(Ast *block);


// Helper function: rewrites a pipeline into a single, nested expression.
// This is the destructive/rewriting pass that fixes the "double free" crash.
Ast *inline_pipe_and_assignment(Ast *expr) {
    if (!expr) return NULL;

    // 1. Optimize BinOps and Calls recursively first (safe modification)
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
    
    // 2. Optimize Pipelines (A |> B(C, D) becomes B(A, C, D))
    if (expr->type == AST_PIPELINE) {
        // LHS (A) is passed for optimization.
        Ast *left_opt = inline_pipe_and_assignment(expr->pipe.left); 
        Ast *right_call = expr->pipe.right;

        if (right_call->type != AST_CALL) {
            // Should not happen if grammar is correct.
            free_ast_node_wrapper(expr);
            return left_opt; 
        }

        // --- Create the new Call Node ---
        
        // 1. Create new argument array: [A, C, D, ...]
        int new_nargs = right_call->call.nargs + 1;
        Ast **new_args = malloc(sizeof(Ast*) * new_nargs);
        if (!new_args) { free_ast_node_wrapper(expr); return left_opt; }

        // The optimized left side (A) becomes the first argument
        new_args[0] = left_opt;

        // The remaining arguments (C, D, ...) follow. Optimize them recursively.
        for (int i = 0; i < right_call->call.nargs; i++) {
            new_args[i + 1] = optimize_expr(right_call->call.args[i]); 
        }
        
        // 2. Create the new combined call node: B(A, C, D)
        // Note: right_call->call.name is strdup'ed when make_call is called.
        Ast *combined_call = make_call(right_call->call.name, new_args, new_nargs);

        // --- Surgical Cleanup (Fixes the crash) ---
        
        // Free the arguments container array from the original RHS call.
        // We do NOT free the content pointers (they were moved to new_args).
        free(right_call->call.args); 
        
        // Free the original RHS call node structure (which contained the name, freed above).
        free_ast_node_wrapper(right_call); 

        // Free the original AST_PIPELINE node structure.
        free_ast_node_wrapper(expr); 

        return combined_call;
    }
    
    // For literals/idents/un-optimized nodes, return the original node.
    return expr;
}

Ast *optimize_expr(Ast *expr) {
    if (!expr) return NULL;
    // Perform the inlining pass
    return inline_pipe_and_assignment(expr);
}

Ast *optimize_decl(Ast *decl) {
    if (!decl) return NULL;
    // The expression is now a single nested call structure
    decl->decl.expr = optimize_expr(decl->decl.expr);
    return decl;
}

Ast *optimize_assign(Ast *assign) {
    if (!assign) return NULL;
    // The expression is now a single nested call structure
    assign->assign.expr = optimize_expr(assign->assign.expr);
    return assign;
}

Ast *optimize_expr_stmt(Ast *expr_stmt) {
    if (!expr_stmt) return NULL;
    // The expression is now a single nested call structure
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
            return optimize_block(stmt); // Recursively optimize blocks
        case AST_IF: // Example: Must optimize the condition and the block
            stmt->if_stmt.cond = optimize_expr(stmt->if_stmt.cond);
            stmt->if_stmt.block = optimize_block(stmt->if_stmt.block);
            return stmt;
        // ... include other statements here if implemented (while, for, return, func_def)
        default:
            return stmt;
    }
}

Ast *optimize_block(Ast *block) {
    if (!block || block->type != AST_BLOCK) return block;

    for (int i = 0; i < block->block.n; i++) {
        // Optimize each statement in place
        block->block.stmts[i] = optimize_stmt(block->block.stmts[i]);
    }
    return block;
}

Ast *optimize_ast(Ast *root) {
    if (!root) return NULL;
    return optimize_block(root);
}