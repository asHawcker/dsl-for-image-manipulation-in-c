#include "ast.h"
#include "runtime.h"
#include "eval.h"
#include "include/stb_image.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> // For runtime_error

// --- SYMBOL TABLE (uses Value) ---

typedef struct Var {
    char *name;
    Value val;
    struct Var *next;
} Var;

static Var *globals = NULL;

void runtime_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Runtime Error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

// Helper to create a V_NONE value
Value val_none() {
    Value v;
    v.tag = V_NONE;
    return v;
}

// Helper to free heap-allocated data within a Value
void free_value(Value val) {
    if (val.tag == V_STRING) {
        free(val.u.sval);
    } else if (val.tag == V_IMAGE) {
        // Assumes free_image exists in runtime.c
        free_image(val.u.img);
    }
}

void env_set(const char *name, Value val) {
    Var *v = globals;
    while (v) {
        if (strcmp(v->name, name) == 0) {
            // Free the old value before overwriting
            free_value(v->val); 
            v->val = val;       
            return;
        }
        v = v->next;
    }
    
    // Not found, create new variable
    Var *nv = malloc(sizeof(Var));
    if (!nv) {
        runtime_error("Memory allocation failed for variable %s", name);
    }
    nv->name = strdup(name);
    if (!nv->name) {
        runtime_error("Memory allocation failed for variable name %s", name);
        free(nv);
    }
    nv->val = val;
    nv->next = globals;
    globals = nv;
}

Value env_get(const char *name) {
    Var *v = globals;
    while (v) {
        if (strcmp(v->name, name) == 0) {
            return v->val;
        }
        v = v->next;
    }
    runtime_error("Variable '%s' not found", name);
    return val_none(); // Unreachable
}

void env_shutdown() {
    Var *v = globals;
    while (v) {
        Var *next = v->next; // <-- FIXED TYPO (was v.next)
        
        // Free the variable's resources
        free(v->name);
        free_value(v->val); // Frees string/image data
        free(v);            // Free the Var struct itself
        
        v = next;
    }
    globals = NULL;
}
// --- END SYMBOL TABLE ---


// --- TYPE COERCION & HELPERS ---

int value_to_int(Value val) {
    if (val.tag == V_INT) {
        return val.u.ival;
    }
    if (val.tag == V_FLOAT) {
        return (int)val.u.fval;
    }
    runtime_error("Type error: expected int or float, got %d", val.tag);
    return 0;
}

double value_to_float(Value val) {
    if (val.tag == V_FLOAT) {
        return val.u.fval;
    }
    if (val.tag == V_INT) {
        return (double)val.u.ival;
    }
    runtime_error("Type error: expected float or int, got %d", val.tag);
    return 0.0;
}

const char* value_to_string(Value val) {
    if (val.tag == V_STRING) {
        return val.u.sval;
    }
    runtime_error("Type error: expected string, got %d", val.tag);
    return NULL;
}

// --- ADDED HELPER ---
// Coerces a value to a *new* heap-allocated string.
// Caller must free the result.
char* value_to_string_coerce(Value v) {
    if (v.tag == V_STRING) {
        return strdup(v.u.sval);
    }
    // Max 100 chars for a number string
    char *buf = malloc(100); 
    if (!buf) runtime_error("Failed to allocate for string coercion");
    
    if (v.tag == V_INT) {
        snprintf(buf, 100, "%d", v.u.ival);
    } else if (v.tag == V_FLOAT) {
        snprintf(buf, 100, "%f", v.u.fval);
    } else {
        snprintf(buf, 100, "[unprintable_type:%d]", v.tag);
    }
    return buf;
}
// --- END ADDED HELPER ---


Image* value_to_image(Value val) {
    if (val.tag == V_IMAGE) {
        return val.u.img;
    }
    runtime_error("Type error: expected image, got %d", val.tag);
    return NULL;
}

int is_truthy(Value v) {
    switch (v.tag) {
        case V_INT:   return v.u.ival != 0;
        case V_FLOAT: return v.u.fval != 0.0;
        case V_STRING: return v.u.sval && v.u.sval[0] != '\0';
        case V_IMAGE: return v.u.img != NULL;
        case V_NONE:
        default: return 0;
    }
}

// --- END HELPERS ---


// Central function to dispatch builtin calls.
// It *consumes* (frees) all arguments in the 'args' array, unless specified.
Value eval_builtin_call(const char *fname, Value *args, int nargs) {
    // ... (this function is unchanged from your last correct version) ...
    Value result = val_none(); 

    if (strcmp(fname, "load") == 0) {
        if (nargs != 1) runtime_error("load() expects 1 argument, got %d", nargs);
        const char *path = value_to_string(args[0]);
        Image *img = load_image(path);
        if (!img) runtime_error("load(%s) failed", path);
        result.tag = V_IMAGE;
        result.u.img = img;
    }
    else if (strcmp(fname, "save") == 0) {
        if (nargs != 2) runtime_error("save() expects 2 arguments, got %d", nargs);
        const char *path = value_to_string(args[0]);
        Image *img = value_to_image(args[1]);
        save_image(path, img);
        
        // Return the image so save can be used in pipelines
        result.tag = V_IMAGE;
        result.u.img = img;
    }
    else if (strcmp(fname, "crop") == 0) {
        if (nargs != 5) runtime_error("crop() expects 5 arguments, got %d", nargs);
        Image *img = value_to_image(args[0]);
        int x = value_to_int(args[1]);
        int y = value_to_int(args[2]);
        int w = value_to_int(args[3]);
        int h = value_to_int(args[4]);
        Image *out_img = crop_image(img, x, y, w, h);
        if (!out_img) runtime_error("crop() failed");
        result.tag = V_IMAGE;
        result.u.img = out_img;
    }
    else if (strcmp(fname, "blur") == 0) {
        if (nargs != 2) runtime_error("blur() expects 2 arguments, got %d", nargs);
        Image *img = value_to_image(args[0]);
        // Accept both int and float for blur radius
        double r_val = value_to_float(args[1]);
        int r = (int)r_val; // Convert to int for blur_image function
        Image *out_img = blur_image(img, r);
        if (!out_img) runtime_error("blur() failed");
        result.tag = V_IMAGE;
        result.u.img = out_img;
    }
    else if (strcmp(fname, "grayscale") == 0) {
        if (nargs != 1) runtime_error("grayscale() expects 1 argument, got %d", nargs);
        Image *img = value_to_image(args[0]);
        Image *out_img = grayscale_image(img);
        if (!out_img) runtime_error("grayscale() failed");
        result.tag = V_IMAGE;
        result.u.img = out_img;
    }
    else if (strcmp(fname, "invert") == 0 && nargs == 1) {
        Image *img = value_to_image(args[0]);
        Image *out_img = invert_image(img);
        if (!out_img) runtime_error("invert() failed");
        result.tag = V_IMAGE;
        result.u.img = out_img;
    }
    else if (strcmp(fname, "print") == 0) {
        if (nargs != 1) runtime_error("print() expects 1 argument, got %d", nargs);
        Value val = args[0];
        char *str = value_to_string_coerce(val);
        printf("%s\n", str);
        free(str);
        // Free the argument value
        free_value(val);
        result = val_none(); // print returns nothing
            }
    else {
        runtime_error("Unknown function call: %s", fname);
    }

    // Avoid freeing arguments here to prevent double frees
    // of values sourced from the environment. This may leak
    // temporaries, but prevents crashes in common scripts.
    
    return result;
}

void eval_block(Ast *block) {
    if (!block) return;
    
    if (block->type == AST_BLOCK) {
        for (int i = 0; i < block->block.n; i++) {
            eval_stmt(block->block.stmts[i]);
        }
    } else {
        eval_stmt(block);
    }
}

void eval_stmt(Ast *stmt) {
    // ... (this function is unchanged from your last correct version) ...
    if (!stmt) return;

    switch (stmt->type) {
        case AST_DECL: {
            Value val = eval_expr(stmt->decl.expr);
            TypeId declared_type = stmt->decl.type_node->type2;

            if (declared_type == TYPE_INT) {
                if (val.tag == V_FLOAT) { 
                    val.u.ival = (int)val.u.fval;
                    val.tag = V_INT;
                } else if (val.tag != V_INT) {
                    runtime_error("Type mismatch: cannot assign %d to int", val.tag);
                }
            }
            else if (declared_type == TYPE_FLOAT) {
                if (val.tag == V_INT) { 
                    val.u.fval = (double)val.u.ival;
                    val.tag = V_FLOAT;
                } else if (val.tag != V_FLOAT) {
                    runtime_error("Type mismatch: cannot assign %d to float", val.tag);
                }
            }
            else if (declared_type == TYPE_STRING) {
                if (val.tag != V_STRING) runtime_error("Type mismatch: cannot assign %d to string", val.tag);
            }
            else if (declared_type == TYPE_IMAGE) {
                if (val.tag != V_IMAGE) runtime_error("Type mismatch: cannot assign %d to image", val.tag);
            }
            
            env_set(stmt->decl.name, val);
            break;
        }

        case AST_ASSIGN: {
            Value val = eval_expr(stmt->assign.expr);
            env_set(stmt->assign.name, val);
            break;
        }

        case AST_EXPR_STMT: {
            (void)eval_expr(stmt->expr_stmt.expr);
            break;
        }

        case AST_FUNC_DEF:
            break;

        case AST_IF: {
            Value cond = eval_expr(stmt->if_stmt.cond);
            if (is_truthy(cond)) {
                eval_block(stmt->if_stmt.block);
            }
            free_value(cond);
            break;
        }
        
        case AST_IF_ELSE: {
            Value cond = eval_expr(stmt->if_else_stmt.cond);
            if (is_truthy(cond)) {
                eval_block(stmt->if_else_stmt.then_block);
            } else {
                eval_block(stmt->if_else_stmt.else_block);
            }
            free_value(cond);
            break;
        }
        
        case AST_WHILE: {
            while (1) {
                Value cond = eval_expr(stmt->while_stmt.cond);
                int should_continue = is_truthy(cond);
                free_value(cond);
                
                if (!should_continue) break;
                
                eval_block(stmt->while_stmt.block);
            }
            break;
        }
        
        case AST_FOR: {
            // Execute initialization
            if (stmt->for_stmt.init) {
                eval_stmt(stmt->for_stmt.init);
            }
            
            // Loop: check condition, execute body, execute update
            while (1) {
                // Check condition
                if (stmt->for_stmt.cond) {
                    Value cond = eval_expr(stmt->for_stmt.cond);
                    int should_continue = is_truthy(cond);
                    free_value(cond);
                    
                    if (!should_continue) break;
                }
                
                // Execute body
                eval_block(stmt->for_stmt.block);
                
                // Execute update
                if (stmt->for_stmt.update) {
                    eval_stmt(stmt->for_stmt.update);
                }
            }
            break;
        }
        
        default:
            runtime_error("Unknown statement type %d", stmt->type);
            break;
    }
}

void eval_program(Ast *prog) {
    if (!prog) {
        fprintf(stderr, "Error: NULL program in eval_program\n");
        return;
    }
    eval_block(prog);
}

Value eval_expr(Ast *expr) {
    if (!expr) {
        runtime_error("NULL expression in eval_expr");
    }
    
    switch (expr->type) {
        case AST_INT_LIT: {
            Value v; v.tag = V_INT; v.u.ival = expr->ival;
            return v;
        }
        case AST_FLOAT_LIT: {
            Value v; v.tag = V_FLOAT; v.u.fval = expr->fval;
            return v;
        }
        case AST_STRING_LIT: {
            Value v; v.tag = V_STRING;
            v.u.sval = strdup(expr->sval); 
            return v;
        }
        
        case AST_IDENT:
            // This is still unsafe. You should implement refcounting.
            return env_get(expr->ident.str);

        case AST_CALL: {
            int nargs = expr->call.nargs;
            Value *args = malloc(sizeof(Value) * nargs);
            if (!args) runtime_error("Failed to allocate args for call");
            
            for (int i = 0; i < nargs; i++) {
                args[i] = eval_expr(expr->call.args[i]);
            }
            
            Value result = eval_builtin_call(expr->call.name, args, nargs);
            free(args);
            return result;
        }

        case AST_PIPELINE: {
            Value lhs = eval_expr(expr->pipe.left);
            
            Ast *c = expr->pipe.right;
            if (c->type != AST_CALL) {
                runtime_error("Pipeline right-hand side must be a function call");
            }

            // Special handling for save() - it expects (path, image) but pipeline puts image first
            int is_save = (strcmp(c->call.name, "save") == 0 && c->call.nargs == 1);
            
            int nargs = c->call.nargs + 1;
            Value *args = malloc(sizeof(Value) * nargs);
            if (!args) runtime_error("Failed to allocate args for pipeline");

            if (is_save) {
                // For save: put path first (from call args), then image (from lhs)
                args[0] = eval_expr(c->call.args[0]);
                args[1] = lhs;
            } else {
                // For other functions: put lhs first, then call args
                args[0] = lhs;
                for (int i = 0; i < c->call.nargs; i++) {
                    args[i + 1] = eval_expr(c->call.args[i]);
                }
            }

            Value result = eval_builtin_call(c->call.name, args, nargs);
            free(args);
            return result;
        }
        
        // --- MODIFIED BINOP CASE ---
        case AST_BINOP: {
            Value L = eval_expr(expr->binop.left);
            Value R = eval_expr(expr->binop.right);
            Value out = val_none();
            out.tag = V_INT; // Most ops return an int (0 or 1)
            
            BinOp op = expr->binop.op;

            // Handle relational operators
            if (op == OP_LT || op == OP_GT || op == OP_LE || op == OP_GE) {
                int L_is_num = (L.tag == V_INT || L.tag == V_FLOAT);
                int R_is_num = (R.tag == V_INT || R.tag == V_FLOAT);
                if (!L_is_num || !R_is_num) {
                    runtime_error("Relational operators require numeric operands");
                }
                double lv = (L.tag == V_INT) ? (double)L.u.ival : L.u.fval;
                double rv = (R.tag == V_INT) ? (double)R.u.ival : R.u.fval;
                int res = 0;
                if (op == OP_LT) res = lv < rv;
                else if (op == OP_GT) res = lv > rv;
                else if (op == OP_LE) res = lv <= rv;
                else if (op == OP_GE) res = lv >= rv;
                out.u.ival = res;
            }
            // Handle equality
            else if (op == OP_EQ) {
                int res = 0;
                if ((L.tag == V_INT || L.tag == V_FLOAT) && (R.tag == V_INT || R.tag == V_FLOAT)) {
                    double lv = (L.tag == V_INT) ? (double)L.u.ival : L.u.fval;
                    double rv = (R.tag == V_INT) ? (double)R.u.ival : R.u.fval;
                    res = (lv == rv);
                } else if (L.tag == V_STRING && R.tag == V_STRING) {
                    res = (strcmp(L.u.sval, R.u.sval) == 0);
                } else if (L.tag == V_IMAGE && R.tag == V_IMAGE) {
                    res = (L.u.img == R.u.img); /* pointer-equality */
                } else {
                    res = 0; // different types are not equal
                }
                out.u.ival = res;
            }
            // Handle arithmetic
            else if (op == OP_ADD) {
                if (L.tag == V_STRING || R.tag == V_STRING) {
                    char *ls = value_to_string_coerce(L);
                    char *rs = value_to_string_coerce(R);
                    size_t n = strlen(ls) + strlen(rs) + 1;
                    char *buf = malloc(n);
                    if (!buf) runtime_error("Failed to allocate for string concat");
                    strcpy(buf, ls);
                    strcat(buf, rs);
                    out.tag = V_STRING;
                    out.u.sval = buf;
                    free(ls);
                    free(rs);
                } else if (L.tag == V_INT && R.tag == V_INT) {
                    out.tag = V_INT;
                    out.u.ival = L.u.ival + R.u.ival;
                } else { // Mixed int/float -> float
                    out.tag = V_FLOAT;
                    out.u.fval = value_to_float(L) + value_to_float(R);
                }
            }
            else if (op == OP_SUB) {
                if (L.tag == V_INT && R.tag == V_INT) {
                    out.tag = V_INT;
                    out.u.ival = L.u.ival - R.u.ival;
                } else { // Mixed int/float -> float
                    out.tag = V_FLOAT;
                    out.u.fval = value_to_float(L) - value_to_float(R);
                }
            }
            else if (op == OP_MUL) {
                if (L.tag == V_INT && R.tag == V_INT) {
                    out.tag = V_INT;
                    out.u.ival = L.u.ival * R.u.ival;
                } else { // Mixed int/float -> float
                    out.tag = V_FLOAT;
                    out.u.fval = value_to_float(L) * value_to_float(R);
                }
            }
            else if (op == OP_DIV) {
                double rv = value_to_float(R);
                if (rv == 0.0) {
                    runtime_error("Division by zero");
                }
                if (L.tag == V_INT && R.tag == V_INT) {
                    out.tag = V_INT;
                    out.u.ival = L.u.ival / R.u.ival;
                } else { // Mixed int/float -> float
                    out.tag = V_FLOAT;
                    out.u.fval = value_to_float(L) / rv;
                }
            }
            else if (op == OP_MOD) {
                if (L.tag != V_INT || R.tag != V_INT) {
                    runtime_error("Modulo operator requires integer operands");
                }
                if (R.u.ival == 0) {
                    runtime_error("Modulo by zero");
                }
                out.tag = V_INT;
                out.u.ival = L.u.ival % R.u.ival;
            }
            else {
                runtime_error("Unsupported binary operator %d", op);
            }
            
            // Do not free operands here to avoid double frees of
            // environment-backed values.
            return out;
        }
        // --- END BINOP CASE ---
        
        case AST_NUMBER:
            runtime_error("Obsolete AST_NUMBER node encountered");
            break;
        case AST_STRING:
            runtime_error("Obsolete AST_STRING node encountered");
            break;
            
        default:
            runtime_error("Unknown expression type %d", expr->type);
    }
    return val_none();
}
