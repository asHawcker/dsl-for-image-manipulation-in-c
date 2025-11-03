#include "ast.h"
#include "runtime.h"
#include "eval.h"
#include "include/stb_image.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> // For runtime_error
#include "parser.tab.h"

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

Image *copy_image(Image *img) {
    if (!img) {
        runtime_error("copy_image: received NULL image");
        return NULL;
    }
    if (!img->data) {
         runtime_error("copy_image: received image with NULL data");
         return NULL;
    }
    Image *new_img = malloc(sizeof(Image));
    if (!new_img) {
        runtime_error("copy_image: failed to allocate new Image struct");
        return NULL;
    }
    new_img->width = img->width;
    new_img->height = img->height;
    new_img->channels = img->channels;
    size_t data_size = (size_t)img->width * img->height * img->channels;
    if (data_size == 0) {
        runtime_error("copy_image: image has zero size");
        free(new_img);
        return NULL;
    }
    new_img->data = malloc(data_size);
    if (!new_img->data) {
        runtime_error("copy_image: failed to allocate new image data");
        free(new_img);
        return NULL;
    }
    memcpy(new_img->data, img->data, data_size);
    return new_img;
}

Value value_clone(Value val) {
    if (val.tag == V_STRING) {
        Value new_val;
        new_val.tag = V_STRING;
        new_val.u.sval = strdup(val.u.sval);
        if (!new_val.u.sval) runtime_error("Failed to clone string");
        return new_val;
    }
    if (val.tag == V_IMAGE) {
        Value new_val;
        new_val.tag = V_IMAGE;
        new_val.u.img = copy_image(val.u.img);
        if (!new_val.u.img) runtime_error("Failed to clone image");
        return new_val;
    }
    return val;
}

// --- END HELPERS ---


// Central function to dispatch builtin calls.
// It *consumes* (frees) all arguments in the 'args' array, unless specified.
Value eval_builtin_call(const char *fname, Value *args, int nargs) {
    Value result = val_none(); // Default return
    int free_args = 1;

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
        
        free_value(args[0]);
        free_args = 0;
        
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
    else if (strcmp(fname, "invert") == 0 && nargs == 1) {
        Image *img = value_to_image(args[0]);
        Image *out_img = invert_image(img);
        if (!out_img) runtime_error("invert() failed");
        result.tag = V_IMAGE;
        result.u.img = out_img;
    }else if (strcmp(fname, "contrast") == 0) {
        if (nargs != 3) runtime_error("contrast() expects 3 arguments, got %d", nargs);
        
        Image *img = value_to_image(args[0]);
        int amount = value_to_int(args[1]);
        int direction = value_to_int(args[2]);

        if (direction != 0 && direction != 1) {
            runtime_error("contrast() direction (arg 3) must be 0 (reduce) or 1 (increase), got %d", direction);
        }
        if (amount < 0 || amount > 100) {
            fprintf(stderr, "Warning: contrast amount %d is outside recommended 0-100 range. Clamping.\n", amount);
            if (amount < 0) amount = 0;
            if (amount > 100) amount = 100;
        }
        Image *out_img = adjust_contrast(img, amount, direction);
        if (!out_img) runtime_error("contrast() failed");

        result.tag = V_IMAGE;
        result.u.img = out_img;
    } else if (strcmp(fname, "brighten") == 0) {
        if (nargs != 3) runtime_error("brighten() expects 3 arguments, got %d", nargs);
        
        Image *img = value_to_image(args[0]);
        int bias = value_to_int(args[1]);
        int direction = value_to_int(args[2]);

        if (direction != 0 && direction != 1) {
            runtime_error("brighten() direction (arg 3) must be 0 (reduce) or 1 (increase), got %d", direction);
        }

        Image *out_img = adjust_brightness(img, bias, direction);
        if (!out_img) runtime_error("brighten() failed");
        
        result.tag = V_IMAGE;
        result.u.img = out_img;
    } else if (strcmp(fname, "threshold") == 0) {
        if (nargs != 3) runtime_error("threshold() expects 3 arguments, got %d", nargs);
        
        Image *img = value_to_image(args[0]);
        int threshold = value_to_int(args[1]);
        int direction = value_to_int(args[2]);

        if (direction != 0 && direction != 1) {
            runtime_error("threshold() direction (arg 3) must be 0 (inverted) or 1 (standard), got %d", direction);
        }

        if (threshold < 0 || threshold > 255) {
            runtime_error("threshold() value (arg 2) must be between 0 and 255, got %d", threshold);
        }
        Image *out_img = apply_threshold(img, threshold, direction);
        if (!out_img) runtime_error("threshold() failed");
        
        result.tag = V_IMAGE;
        result.u.img = out_img;
    } else if (strcmp(fname, "sharpen") == 0) {
        if (nargs != 3) runtime_error("sharpen() expects 3 arguments, got %d", nargs);
        
        Image *img = value_to_image(args[0]);
        int amount = value_to_int(args[1]);
        int direction = value_to_int(args[2]);

        if (direction != 0 && direction != 1) {
            runtime_error("sharpen() direction (arg 3) must be 0 (soften) or 1 (sharpen), got %d", direction);
        }
        if (amount < 0) {
            fprintf(stderr, "Warning: sharpen amount %d is negative, using 0.\n", amount);
            amount = 0;
        }
        
        if (direction == 1 && amount > 20) {
                fprintf(stderr, "Warning: sharpen amount %d is very high, capping at 20.\n", amount);
                amount = 20;
        }
        
        if (direction == 0 && amount == 0) {
            amount = 1; 
        }

        Image *out_img = sharpen_image(img, amount, direction);
        if (!out_img) runtime_error("sharpen() failed");
        
        result.tag = V_IMAGE;
        result.u.img = out_img;
    } else if (strcmp(fname, "blend") == 0) {
        if (nargs != 3) runtime_error("blend() expects 3 arguments, got %d", nargs);
        
        Image *img1 = value_to_image(args[0]);
        Image *img2 = value_to_image(args[1]);
        float alpha = (float)value_to_float(args[2]);

        if (alpha < 0.0f || alpha > 1.0f) {
            fprintf(stderr, "Warning: blend() alpha %f is outside [0.0, 1.0], clamping.\n", alpha);
            if (alpha < 0.0f) alpha = 0.0f;
            if (alpha > 1.0f) alpha = 1.0f;
        }

        Image *out_img = blend_images(img1, img2, alpha);
        if (!out_img) runtime_error("blend() failed (check image dimensions match)");
        
        result.tag = V_IMAGE;
        result.u.img = out_img;
    } else if (strcmp(fname, "mask") == 0) {
        if (nargs != 2) runtime_error("mask() expects 2 arguments, got %d", nargs);
        
        Image *img = value_to_image(args[0]);
        Image *mask = value_to_image(args[1]);

        Image *out_img = mask_image(img, mask);
        if (!out_img) runtime_error("mask() failed (check image dimensions match)");
        
        result.tag = V_IMAGE;
        result.u.img = out_img;
    } else if (strcmp(fname, "resize") == 0) {
        if (nargs != 3) runtime_error("resize() expects 3 arguments, got %d", nargs);
        
        Image *img = value_to_image(args[0]);
        int w = value_to_int(args[1]);
        int h = value_to_int(args[2]);
        Image *out_img = resize_image_nearest(img, w, h);
        if (!out_img) runtime_error("resize() failed");
         
        result.tag = V_IMAGE;
        result.u.img = out_img;
    } else if (strcmp(fname, "scale") == 0) {
        if (nargs != 2) runtime_error("scale() expects 2 arguments (img, factor), got %d", nargs);
        
        Image *img = value_to_image(args[0]);
        float factor = value_to_float(args[1]);

        Image *out_img = scale_image_factor(img, factor);
        if (!out_img) runtime_error("scale() failed");
        
        result.tag = V_IMAGE;
        result.u.img = out_img;
    } else if (strcmp(fname, "rotate") == 0) {
        if (nargs != 2) runtime_error("rotate() expects 2 arguments (img, angle_degrees), got %d", nargs);
        
        Image *img = value_to_image(args[0]);
        int direction = value_to_int(args[1]);

        Image *out_img = rotate_image_90(img, direction);
        if (!out_img) runtime_error("rotate() failed");
        
        result.tag = V_IMAGE;
        result.u.img = out_img;
    } else if (strcmp(fname, "print") == 0) {
        for (int i = 0; i < nargs; i++) {
            switch (args[i].tag) {
                case V_IMAGE:
                    printf("<Image %dx%d>", args[i].u.img->width, args[i].u.img->height);
                    break;
                case V_INT:
                    printf("%d", args[i].u.ival);
                    break;
                case V_FLOAT:
                    printf("%f", args[i].u.fval);
                    break;
                case V_STRING:
                    print_string_escaped(args[i].u.sval);
                    break;
                default:
                    printf("<unknown_type>");
                    break;
            }
            
            // if (i < nargs - 1) {
            //     print_string_escaped(" ");
            // }
        }
        // print_string_escaped("\n"); 
        
        result.tag = V_NONE;
    }
    else {
        runtime_error("Unknown function call: %s", fname);
    }

    if (free_args) {
        for (int i = 0; i < nargs; i++) {
            free_value(args[i]);
        }
    }
    
    return result;
}

void eval_block(Ast *block) {
    if (!block) {
        runtime_error("eval_block: received NULL block");
        return;
    }
    if (block->type != AST_BLOCK) {
        runtime_error("eval_block: expected AST_BLOCK, got %d", block->type);
        return;
    }

    // Loop through and evaluate each statement in the block
    for (int i = 0; i < block->block.n; i++) {
        eval_stmt(block->block.stmts[i]);
    }
}

void eval_stmt(Ast *stmt) {
    if (!stmt) return;

    switch (stmt->type) {
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
        case AST_IF: {
            Value cond_val = eval_expr(stmt->if_stmt.cond);
            
            int is_true = value_to_int(cond_val);
            
            free_value(cond_val);

            if (is_true) {
                eval_block(stmt->if_stmt.block);
            }
            break;
        }

        case AST_IF_ELSE: {
            Value cond_val = eval_expr(stmt->if_else_stmt.cond);
            
            int is_true = value_to_int(cond_val);
            
            free_value(cond_val);

            if (is_true) {
                eval_block(stmt->if_else_stmt.then_block);
            } else {
                eval_block(stmt->if_else_stmt.else_block);
            }
            break;
        }

        case AST_WHILE: {
            while (1) {
                Value cond_val = eval_expr(stmt->while_stmt.cond);
                int is_true = value_to_int(cond_val);
                free_value(cond_val);

                if (!is_true) {
                    break;
                }
                eval_block(stmt->while_stmt.block);
            }
            break;
        }

        case AST_FOR: {
            if (stmt->for_stmt.init) {
                eval_stmt(stmt->for_stmt.init);
            }

            while (1) {
                Value cond_val = eval_expr(stmt->for_stmt.cond);
                int is_true = value_to_int(cond_val);
            
                free_value(cond_val);

                if (!is_true) {
                    break;
                }
                eval_block(stmt->for_stmt.block);
                if (stmt->for_stmt.update) {
                    eval_stmt(stmt->for_stmt.update);
                }
            }
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
    env_shutdown();
}

Value eval_expr(Ast *expr) {
    if (!expr) {
        runtime_error("NULL expression in eval_expr");
    }
    
    switch (expr->type) {
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
        
        case AST_IDENT:
            return value_clone(env_get(expr->ident.str));

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

        case AST_BINOP: {
            Value left = eval_expr(expr->binop.left);
            Value right = eval_expr(expr->binop.right);
            Value result;
            
            int op = expr->binop.op;

            if (left.tag == V_STRING || right.tag == V_STRING) {
                if (left.tag != V_STRING || right.tag != V_STRING) {
                     runtime_error("Operator %d requires both operands to be strings, or neither.", op);
                }
                
                if (op == PLUS) {
                    const char *l = left.u.sval;
                    const char *r = right.u.sval;
                    size_t len = strlen(l) + strlen(r);
                    char *new_s = malloc(len + 1);
                    if (!new_s) runtime_error("Failed to allocate for string concatenation");
                    strcpy(new_s, l);
                    strcat(new_s, r);
                    
                    result.tag = V_STRING;
                    result.u.sval = new_s;
                } else if (op == EQ) {
                    result.tag = V_INT;
                    result.u.ival = (strcmp(left.u.sval, right.u.sval) == 0);
                } else if (op == NEQ) {
                    result.tag = V_INT;
                    result.u.ival = (strcmp(left.u.sval, right.u.sval) != 0);
                } else {
                    runtime_error("Operator %d not supported for string types", op);
                }
            }
            else if (left.tag == V_FLOAT || right.tag == V_FLOAT) {
                double l = value_to_float(left); 
                double r = value_to_float(right);

                switch (op) {
                    case PLUS:  result.tag = V_FLOAT; result.u.fval = l + r; break;
                    case MINUS: result.tag = V_FLOAT; result.u.fval = l - r; break;
                    case MUL:   result.tag = V_FLOAT; result.u.fval = l * r; break;
                    case DIV:
                        if (r == 0.0) runtime_error("Division by zero");
                        result.tag = V_FLOAT; result.u.fval = l / r; break;
                    
                    case EQ:  result.tag = V_INT; result.u.ival = (l == r); break;
                    case NEQ: result.tag = V_INT; result.u.ival = (l != r); break;
                    case GT:  result.tag = V_INT; result.u.ival = (l > r); break;
                    case LT:  result.tag = V_INT; result.u.ival = (l < r); break;
                    case GE:  result.tag = V_INT; result.u.ival = (l >= r); break;
                    case LE:  result.tag = V_INT; result.u.ival = (l <= r); break;

                    case MOD:
                        runtime_error("Modulo operator (%%) not supported for floats");
                        break;
                    default:
                        runtime_error("Unknown binary operator %d for floats", op);
                }
            }
            else if (left.tag == V_INT && right.tag == V_INT) {
                int l = left.u.ival;
                int r = right.u.ival;
                result.tag = V_INT;

                switch (op) {
                    case PLUS:  result.u.ival = l + r; break;
                    case MINUS: result.u.ival = l - r; break;
                    case MUL:   result.u.ival = l * r; break;
                    case DIV:
                        if (r == 0) runtime_error("Division by zero");
                        result.u.ival = l / r; break;
                    case MOD:
                        if (r == 0) runtime_error("Modulo by zero");
                        result.u.ival = l % r; break;
                    case EQ:  result.u.ival = (l == r); break;
                    case NEQ: result.u.ival = (l != r); break;
                    case GT:  result.u.ival = (l > r); break;
                    case LT:  result.u.ival = (l < r); break;
                    case GE:  result.u.ival = (l >= r); break;
                    case LE:  result.u.ival = (l <= r); break;

                    default:
                        runtime_error("Unknown binary operator %d for ints", op);
                }
            }
            else {
                runtime_error("Binary operator %d not supported for types %d and %d", op, left.tag, right.tag);
            }
            free_value(left);
            free_value(right);
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
