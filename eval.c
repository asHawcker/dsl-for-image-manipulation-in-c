#include "ast.h"
#include "runtime.h"
#include "eval.h"
#include "include/stb_image.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> // For runtime_error

// --- NEW SYMBOL TABLE (uses Value) ---

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
            free_value(v->val); // <-- FIX: was v.val
            v->val = val;       // <-- FIX: was v.val
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

// --- END NEW SYMBOL TABLE ---


// --- NEW TYPE COERCION HELPERS ---

int value_to_int(Value val) {
    if (val.tag == V_INT) {
        return val.u.ival;
    }
    if (val.tag == V_FLOAT) {
        // Allow float -> int coercion
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
        // Allow int -> float coercion
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

Image* value_to_image(Value val) {
    if (val.tag == V_IMAGE) {
        return val.u.img;
    }
    runtime_error("Type error: expected image, got %d", val.tag);
    return NULL;
}

// --- END HELPERS ---


// Central function to dispatch builtin calls.
// It *consumes* (frees) all arguments in the 'args' array, unless specified.
Value eval_builtin_call(const char *fname, Value *args, int nargs) {
    Value result = val_none(); // Default return
    int free_args = 1;         // --- ADDED --- By default, free all args

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
        
        // --- ADDED SPECIAL FREEING ---
        // 'save' consumes the path string, but *not* the image Value.
        // We only free the path, and tell the main loop to skip freeing.
        free_value(args[0]); // free the path string
        free_args = 0;       // Tell the final loop not to run
        // --- END SPECIAL FREEING ---
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
        int r = value_to_int(args[1]);
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
    // ... other functions (invert, flipX, flipY, cannyedge, brighten) ...
    // Example for 'invert'
    else if (strcmp(fname, "invert") == 0 && nargs == 1) {
        Image *img = value_to_image(args[0]);
        Image *out_img = invert_image(img);
        if (!out_img) runtime_error("invert() failed");
        result.tag = V_IMAGE;
        result.u.img = out_img;
    }
    else {
        runtime_error("Unknown function call: %s", fname);
    }

    // --- CHANGED ---
    // Free all arguments *if* not handled specially
    if (free_args) {
        for (int i = 0; i < nargs; i++) {
            free_value(args[i]);
        }
    }
    
    return result;
}

void eval_stmt(Ast *stmt) {
    if (!stmt) return;

    switch (stmt->type) {
        // === THIS IS THE REQUESTED IMPLEMENTATION ===
        case AST_DECL: {
            // 1. Evaluate the expression
            Value val = eval_expr(stmt->decl.expr);
            TypeId declared_type = stmt->decl.type_node->type2;

            // 2. Type-check and coerce
            if (declared_type == TYPE_INT) {
                if (val.tag == V_FLOAT) { // Coerce float to int
                    val.u.ival = (int)val.u.fval;
                    val.tag = V_INT;
                } else if (val.tag != V_INT) {
                    runtime_error("Type mismatch: cannot assign %d to int", val.tag);
                }
            }
            else if (declared_type == TYPE_FLOAT) {
                if (val.tag == V_INT) { // Coerce int to float
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
            
            // 3. Store in environment
            env_set(stmt->decl.name, val);
            break;
        }
        // === END REQUESTED IMPLEMENTATION ===

        case AST_ASSIGN: {
            Value val = eval_expr(stmt->assign.expr);
            // In a strongly typed system, we would check the variable's existing type.
            // For now, we just overwrite.
            env_set(stmt->assign.name, val);
            break;
        }

        case AST_EXPR_STMT: {
            // Evaluate expression and free the result
            Value val = eval_expr(stmt->expr_stmt.expr);
            free_value(val);
            break;
        }

        case AST_FUNC_DEF:
            /* Store function if implementing calls to user funcs */
            break;

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
    for (int i = 0; i < prog->block.n; i++) {
        eval_stmt(prog->block.stmts[i]);
    }
    
    // TODO: Free global environment
}

Value eval_expr(Ast *expr) {
    if (!expr) {
        runtime_error("NULL expression in eval_expr");
    }
    
    switch (expr->type) {
        // === THESE ARE THE REQUESTED IMPLEMENTATIONS ===
        case AST_INT_LIT: {
            Value v;
            v.tag = V_INT;
            v.u.ival = expr->ival;
            return v;
        }
        case AST_FLOAT_LIT: {
            Value v;
            v.tag = V_FLOAT;
            v.u.fval = expr->fval;
            return v;
        }
        case AST_STRING_LIT: {
            Value v;
            v.tag = V_STRING;
            v.u.sval = strdup(expr->sval); // Value must own its own copy
            return v;
        }
        // === END REQUESTED IMPLEMENTATIONS ===
        
        case AST_IDENT:
            // TODO: We should clone the value if it's heap-allocated
            // For now, just return it. This might cause double-free issues.
            // A proper solution requires reference counting or deep copying.
            // For now, we'll assume env_get returns a value we can use.
            return env_get(expr->ident.str);

        case AST_CALL: {
            // 1. Evaluate all arguments
            int nargs = expr->call.nargs;
            Value *args = malloc(sizeof(Value) * nargs);
            if (!args) runtime_error("Failed to allocate args for call");
            
            for (int i = 0; i < nargs; i++) {
                args[i] = eval_expr(expr->call.args[i]);
            }
            
            // 2. Call the builtin function
            Value result = eval_builtin_call(expr->call.name, args, nargs);
            
            // 3. Free the args array
            free(args);
            
            return result;
        }

        case AST_PIPELINE: {
            // 1. Evaluate LHS
            Value lhs = eval_expr(expr->pipe.left);
            
            Ast *c = expr->pipe.right;
            if (c->type != AST_CALL) {
                runtime_error("Pipeline right-hand side must be a function call");
            }

            // 2. Create new argument list
            int nargs = c->call.nargs + 1;
            Value *args = malloc(sizeof(Value) * nargs);
            if (!args) runtime_error("Failed to allocate args for pipeline");

            // 3. Set LHS as first argument
            args[0] = lhs;
            
            // 4. Evaluate RHS arguments
            for (int i = 0; i < c->call.nargs; i++) {
                args[i + 1] = eval_expr(c->call.args[i]);
            }

            // 5. Call builtin
            Value result = eval_builtin_call(c->call.name, args, nargs);
            
            // 6. Free args array
            free(args);

            return result;
        }
        
        // --- OBSOLETE NODES (should be removed from parser) ---
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

// ... at the end of eval.c ...

void env_shutdown() {
    Var *v = globals;
    while (v) {
        Var *next = v->next;
        
        // Free the variable's resources
        free(v->name);
        free_value(v->val); // Frees string/image data
        free(v);            // Free the Var struct itself
        
        v = next;
    }
    globals = NULL;
}
