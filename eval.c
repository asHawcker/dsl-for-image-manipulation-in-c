#include "ast.h"
#include "runtime.h"
#include "eval.h"
#include "include/stb_image.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct Var {
    char *name;
    Image *img;
    struct Var *next;
} Var;

static Var *globals = NULL;

static void free_image_if_exists(Image *img) {
    if (img && img->data) {
        stbi_image_free(img->data);
        free(img);
    }
}

static void set_var(char *name, Image *img) {
    Var *v = globals;
    while (v) {
        if (strcmp(v->name, name) == 0) {
            free_image_if_exists(v->img);
            v->img = img;
            return;
        }
        v = v->next;
    }
    Var *nv = malloc(sizeof(Var));
    if (!nv) {
        fprintf(stderr, "Error: Memory allocation failed for variable %s\n", name);
        return;
    }
    nv->name = strdup(name);
    if (!nv->name) {
        fprintf(stderr, "Error: Memory allocation failed for variable name %s\n", name);
        free(nv);
        return;
    }
    nv->img = img;
    nv->next = globals;
    globals = nv;
}

static Image *get_var(char *name) {
    Var *v = globals;
    while (v) {
        if (strcmp(v->name, name) == 0) return v->img;
        v = v->next;
    }
    fprintf(stderr, "Error: Variable %s not found\n", name);
    return NULL;
}

static void remove_var(char *name) {
    Var **pp = &globals;
    while (*pp) {
        if (strcmp((*pp)->name, name) == 0) {
            Var *old = *pp;
            *pp = old->next;
            free_image_if_exists(old->img);
            free(old->name);
            free(old);
            return;
        }
        pp = &(*pp)->next;
    }
}

void eval_stmt(Ast *stmt) {
    if (!stmt) {
        fprintf(stderr, "Error: NULL statement in eval_stmt\n");
        return;
    }
    switch (stmt->type) {
        case AST_ASSIGN: {
            Image *img = eval_expr(stmt->assign.expr);
            if (img) {
                set_var(stmt->assign.name, img);
            } else {
                fprintf(stderr, "Error: Assignment to %s failed (NULL expr result)\n", stmt->assign.name);
            }
            break;
        }
        case AST_EXPR_STMT:
            eval_expr(stmt->expr_stmt.expr);
            break;
        case AST_FUNC_DEF:
            /* Store function if implementing calls to user funcs */
            break;
        default:
            fprintf(stderr, "Error: Unknown statement type %d\n", stmt->type);
            break;
    }
}

void eval_program(Ast *prog) {
    if (!prog) {
        fprintf(stderr, "Error: NULL program in eval_program\n");
        return;
    }
    for (int i = 0; i < prog->block.n; i++) eval_stmt(prog->block.stmts[i]);
}

Image *eval_expr(Ast *expr) {
    if (!expr) {
        fprintf(stderr, "Error: NULL expression in eval_expr\n");
        return NULL;
    }
    switch (expr->type) {
        case AST_IDENT:
            return get_var(expr->ident.str);
        case AST_CALL: {
            Image *img = NULL;
            const char *fname = expr->call.name;
            int nargs = expr->call.nargs;
            if (strcmp(fname, "load") == 0 && nargs == 1 && expr->call.args[0]->type == AST_STRING) {
                img = load_image(expr->call.args[0]->string.str);
                if (!img) fprintf(stderr, "Error: load(%s) failed\n", expr->call.args[0]->string.str);
            } else if (strcmp(fname, "save") == 0 && nargs == 2) {
                Image *i = eval_expr(expr->call.args[1]);
                if (i && expr->call.args[0]->type == AST_STRING) {
                    save_image(expr->call.args[0]->string.str, i);
                } else {
                    fprintf(stderr, "Error: save(%s, %p) failed\n",
                            expr->call.args[0]->type == AST_STRING ? expr->call.args[0]->string.str : "invalid",
                            (void*)i);
                }
            } else if (strcmp(fname, "crop") == 0 && nargs == 5) {
                Image *i = eval_expr(expr->call.args[0]);
                if (!i) {
                    fprintf(stderr, "Error: crop input image is NULL\n");
                    return NULL;
                }
                if (expr->call.args[1]->type != AST_NUMBER ||
                    expr->call.args[2]->type != AST_NUMBER ||
                    expr->call.args[3]->type != AST_NUMBER ||
                    expr->call.args[4]->type != AST_NUMBER) {
                    fprintf(stderr, "Error: crop requires 4 number arguments\n");
                    return NULL;
                }
                int x = (int)expr->call.args[1]->number.num;
                int y = (int)expr->call.args[2]->number.num;
                int w = (int)expr->call.args[3]->number.num;
                int h = (int)expr->call.args[4]->number.num;
                img = crop_image(i, x, y, w, h);
                if (!img) {
                    fprintf(stderr, "Error: crop(%d, %d, %d, %d) failed\n", x, y, w, h);
                }
            } else if (strcmp(fname, "blur") == 0 && nargs == 2) {
                Image *i = eval_expr(expr->call.args[0]);
                if (!i) {
                    fprintf(stderr, "Error: blur input image is NULL\n");
                    return NULL;
                }
                if (expr->call.args[1]->type != AST_NUMBER) {
                    fprintf(stderr, "Error: blur requires a number radius\n");
                    return NULL;
                }
                int r = (int)expr->call.args[1]->number.num;
                img = blur_image(i, r);
                if (!img) {
                    fprintf(stderr, "Error: blur(radius=%d) failed\n", r);
                }
            }
            return img;
        }
        case AST_PIPELINE: {
            Image *l = eval_expr(expr->pipe.left);
            if (!l) {
                fprintf(stderr, "Error: Pipeline left operand is NULL\n");
                return NULL;
            }
            if (expr->pipe.right->type == AST_CALL) {
                Ast *c = expr->pipe.right;
                int orig_nargs = c->call.nargs;
                int nargs = orig_nargs + 1;
                Ast **args = malloc(sizeof(Ast *) * nargs);
                if (!args) {
                    fprintf(stderr, "Error: Memory allocation failed for pipeline args\n");
                    return NULL;
                }
                args[0] = make_ident("__pipe_tmp__");
                for (int i = 0; i < orig_nargs; i++) {
                    args[i + 1] = clone_ast(c->call.args[i]);
                }
                Ast *temp_call = make_call(c->call.name, args, nargs);
                if (!temp_call) {
                    free_ast(args[0]);
                    for (int i = 1; i < nargs; i++) free_ast(args[i]);
                    free(args);
                    fprintf(stderr, "Error: Pipeline call creation failed\n");
                    return NULL;
                }
                set_var("__pipe_tmp__", l);
                Image *img = eval_expr(temp_call);
                remove_var("__pipe_tmp__");
                free_ast(temp_call);
                if (!img) {
                    fprintf(stderr, "Error: Pipeline evaluation failed\n");
                }
                return img;
            }
            fprintf(stderr, "Error: Pipeline right operand is not a call\n");
            return NULL;
        }
        case AST_NUMBER:
        case AST_STRING:
            return NULL;
        default:
            fprintf(stderr, "Error: Unknown expression type %d\n", expr->type);
            return NULL;
    }
    return NULL;
}
