#include "codegen.h"
#include "ast.h"
#include "parser.tab.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> 

typedef struct VarEntry {
    char *name;
    TypeId type; 
    struct VarEntry *next;
} VarEntry;

static VarEntry *var_list = NULL;
static int temp_counter = 0; 

static char *code_buffer = NULL;
static size_t buffer_size = 0;
static size_t buffer_used = 0;

static void buffer_printf(const char *format, ...) {
    va_list args;
    
    if (code_buffer == NULL) {
        buffer_size = 4096; 
        code_buffer = (char *)malloc(buffer_size);
        if (!code_buffer) { fprintf(stderr, "OOM"); exit(1); }
    }

    va_start(args, format);
    int needed = vsnprintf(code_buffer + buffer_used, buffer_size - buffer_used, format, args);
    va_end(args);

    if (needed >= 0 && needed >= buffer_size - buffer_used) {
        buffer_size *= 2; 
        if (buffer_size < buffer_used + needed + 1) {
             buffer_size = buffer_used + needed + 1;
        }
        code_buffer = (char *)realloc(code_buffer, buffer_size);
        if (!code_buffer) { fprintf(stderr, "OOM"); exit(1); }

        va_start(args, format);
        vsnprintf(code_buffer + buffer_used, buffer_size - buffer_used, format, args);
        va_end(args);
    }
    
    if (needed > 0) {
        buffer_used += needed;
    }
}

void free_temp_var_name(const char *var_name) {
    if (var_name && strncmp(var_name, "__temp_", 7) == 0) {
        free((char*)var_name);
    }
}


const char* type_to_c_string(TypeId t) {
    switch (t) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "double";
        case TYPE_STRING: return "char*";
        case TYPE_IMAGE: return "Image*";
        default: return "void*";
    }
}

void register_var(const char *name, TypeId t) {
    VarEntry *current = var_list;
    while(current) {
        if (strcmp(current->name, name) == 0) {
            return; 
        }
        current = current->next;
    }

    VarEntry *new_var = (VarEntry *)malloc(sizeof(VarEntry));
    new_var->name = strdup(name);
    new_var->type = t;
    new_var->next = var_list;
    var_list = new_var;
}


void codegen_stmt(Ast *stmt);
void codegen_block(Ast *block);
const char* codegen_expr(Ast *expr);


char* new_temp_var() {
    char *name = (char *)malloc(32);
    if (!name) {
        fprintf(stderr, "Fatal: memory allocation failed for temp var");
        exit(1);
    }
    sprintf(name, "__temp_%d", temp_counter++);
    return name;
}

void write_indent(int indent) {
    buffer_printf("\n");
    for (int i = 0; i < indent; i++) {
        buffer_printf("    ");
    }
}

const char* map_iml_to_c_func(const char *iml_name) {
    if (strcmp(iml_name, "load") == 0) return "load_image";
    if (strcmp(iml_name, "save") == 0) return "save_image";
    if (strcmp(iml_name, "crop") == 0) return "crop_image";
    if (strcmp(iml_name, "blur") == 0) return "blur_image";
    if (strcmp(iml_name, "grayscale") == 0) return "grayscale_image";
    if (strcmp(iml_name, "invert") == 0) return "invert_image";
    if (strcmp(iml_name, "contrast") == 0) return "adjust_contrast";
    if (strcmp(iml_name, "brighten") == 0) return "adjust_brightness";
    if (strcmp(iml_name, "threshold") == 0) return "apply_threshold"; 
    if (strcmp(iml_name, "sharpen") == 0) return "sharpen_image";
    if (strcmp(iml_name, "blend") == 0) return "blend_images";
    if (strcmp(iml_name, "mask") == 0) return "mask_image"; 
    if (strcmp(iml_name, "resize") == 0) return "resize_image_nearest";
    if (strcmp(iml_name, "scale") == 0) return "scale_image_factor";
    if (strcmp(iml_name, "rotate") == 0) return "rotate_image_90";
    if (strcmp(iml_name, "print") == 0) return "print_string_escaped";
    
    return iml_name; 
}

TypeId get_builtin_return_type(const char *fname) {
    if (strcmp(fname, "save") == 0) return TYPE_UNKNOWN;
    if (strcmp(fname, "print") == 0) return TYPE_UNKNOWN;

    if (strcmp(fname, "load") == 0 || strcmp(fname, "crop") == 0 ||
        strcmp(fname, "blur") == 0 || strcmp(fname, "grayscale") == 0 ||
        strcmp(fname, "invert") == 0 || strcmp(fname, "contrast") == 0 ||
        strcmp(fname, "brighten") == 0 || strcmp(fname, "threshold") == 0 ||
        strcmp(fname, "sharpen") == 0 || strcmp(fname, "blend") == 0 ||
        strcmp(fname, "mask") == 0 || strcmp(fname, "resize") == 0 ||
        strcmp(fname, "scale") == 0 || strcmp(fname, "rotate") == 0) 
    {
        return TYPE_IMAGE;
    }
    return TYPE_INT; 
}


const char* codegen_expr(Ast *expr) {
    if (!expr) {
        fprintf(stderr, "Fatal: NULL expression in codegen_expr");
        return "NULL";
    }
    
    char *result_var;

    switch (expr->type) {
        case AST_INT_LIT: {
            result_var = new_temp_var();
            buffer_printf("int %s = %d;", result_var, expr->ival);
            return result_var;
        }
        case AST_FLOAT_LIT: {
            result_var = new_temp_var();
            buffer_printf("double %s = %lf;", result_var, expr->fval);
            return result_var;
        }
        case AST_STRING_LIT: {
            result_var = new_temp_var();
            buffer_printf("char* %s = \"%s\";", result_var, expr->sval);
            return result_var;
        }
        case AST_NULL_LIT: {
            return "NULL";
        }
        
        case AST_IDENT: {
            return expr->ident.str;
        }

        case AST_CALL: {
            const char **arg_vars = (const char **)malloc(sizeof(char*) * expr->call.nargs);
            if (!arg_vars) { fprintf(stderr, "OOM"); exit(1); }
            for (int i = 0; i < expr->call.nargs; i++) {
                arg_vars[i] = codegen_expr(expr->call.args[i]);
            }
            
            TypeId return_type = get_builtin_return_type(expr->call.name);
            result_var = new_temp_var();
            const char *c_func_name = map_iml_to_c_func(expr->call.name); 
            
            if (return_type == TYPE_UNKNOWN) {
                buffer_printf("%s(", c_func_name); 
            } else {
                buffer_printf("%s %s = %s(", type_to_c_string(return_type), result_var, c_func_name);
            }
            
            for (int i = 0; i < expr->call.nargs; i++) {
                buffer_printf("%s", arg_vars[i]);
                if (i < expr->call.nargs - 1) buffer_printf(", ");
            }
            buffer_printf(");");
            
            for (int i = 0; i < expr->call.nargs; i++) {
                free_temp_var_name(arg_vars[i]); 
            }
            free(arg_vars);

            if (return_type == TYPE_UNKNOWN) {
                free(result_var);
                return "";
            }
            
            return result_var;
        }

        case AST_BINOP: {
            const char *left_var = codegen_expr(expr->binop.left);
            const char *right_var = codegen_expr(expr->binop.right);
            
            TypeId result_type = TYPE_INT;
            const char *op_str;
            switch(expr->binop.op) {
                case PLUS: op_str = "+"; result_type = TYPE_FLOAT; break;
                case MINUS: op_str = "-"; result_type = TYPE_FLOAT; break;
                case MUL: op_str = "*"; result_type = TYPE_FLOAT; break;
                case DIV: op_str = "/"; result_type = TYPE_FLOAT; break;
                case MOD: op_str = "%%"; result_type = TYPE_INT; break;
                case EQ: op_str = "=="; result_type = TYPE_INT; break;
                case NEQ: op_str = "!="; result_type = TYPE_INT; break;
                case GT: op_str = ">"; result_type = TYPE_INT; break;
                case LT: op_str = "<"; result_type = TYPE_INT; break;
                case GE: op_str = ">="; result_type = TYPE_INT; break;
                case LE: op_str = "<="; result_type = TYPE_INT; break;
                default: op_str = "?";
            }
            
            result_var = new_temp_var();
            buffer_printf("%s %s = %s %s %s;", type_to_c_string(result_type), result_var, left_var, op_str, right_var);
            
            free_temp_var_name(left_var);   
            free_temp_var_name(right_var);  
            return result_var;
        }

        case AST_PIPELINE: {
            const char *lhs_var = codegen_expr(expr->pipe.left);
            
            Ast *c = expr->pipe.right;
            if (c->type != AST_CALL) {
                fprintf(stderr, "Fatal: Pipeline right-hand side is not a function call");
                return "NULL";
            }
            
            int nargs_rhs = c->call.nargs;
            const char **arg_vars = (const char **)malloc(sizeof(char*) * nargs_rhs);
            if (!arg_vars) { fprintf(stderr, "OOM"); exit(1); }

            for (int i = 0; i < nargs_rhs; i++) {
                arg_vars[i] = codegen_expr(c->call.args[i]);
            }
            
            TypeId return_type = get_builtin_return_type(c->call.name); 
            result_var = new_temp_var();
            const char *c_func_name = map_iml_to_c_func(c->call.name); 

            if (return_type == TYPE_UNKNOWN) {
                buffer_printf("%s(%s", c_func_name, lhs_var);
            } else {
                buffer_printf("%s %s = %s(%s", type_to_c_string(return_type), result_var, c_func_name, lhs_var);
            }
            
            free_temp_var_name(lhs_var); 

            for (int i = 0; i < nargs_rhs; i++) {
                buffer_printf(", %s", arg_vars[i]);
            }
            buffer_printf(");");
            
            for (int i = 0; i < nargs_rhs; i++) {
                free_temp_var_name(arg_vars[i]); 
            }
            free(arg_vars);

            if (return_type == TYPE_UNKNOWN) {
                free(result_var);
                return "";
            }

            return result_var;
        }
        
        default:
            fprintf(stderr, "Fatal: Unknown expression type %d in codegen_expr", expr->type);
            exit(1);
    }
    return "NULL";
}


void codegen_stmt(Ast *stmt) {
    if (!stmt) return;

    int current_indent = 1; 
    
    switch (stmt->type) {
        case AST_DECL: {
            register_var(stmt->decl.name, stmt->decl.type_node->type2);

            const char *val_var = codegen_expr(stmt->decl.expr);
            write_indent(current_indent);
            buffer_printf("%s = %s;", stmt->decl.name, val_var); 
            free_temp_var_name(val_var); 
            break;
        }

        case AST_ASSIGN: {
            register_var(stmt->assign.name, TYPE_IMAGE); 

            const char *val_var = codegen_expr(stmt->assign.expr);
            write_indent(current_indent);
            buffer_printf("%s = %s;", stmt->assign.name, val_var);
            free_temp_var_name(val_var); 
            break;
        }

        case AST_EXPR_STMT: {
            const char *val_var = codegen_expr(stmt->expr_stmt.expr);
            write_indent(current_indent);
            free_temp_var_name(val_var); 
            break;
        }
        
        default:
            write_indent(current_indent);
            buffer_printf("// Statement type %d not yet implemented in codegen.\n", stmt->type);
            break;
    }
}

void codegen_block(Ast *block) {
    if (!block || block->type != AST_BLOCK) {
        fprintf(stderr, "Fatal: codegen_block received non-block AST");
        return;
    }
    
    for (int i = 0; i < block->block.n; i++) {
        codegen_stmt(block->block.stmts[i]);
    }
}


void codegen_program(Ast *prog, const char *output_c_filename) {
    FILE *outfile = fopen(output_c_filename, "w");
    if (!outfile) {
        perror("Failed to open output C file");
        exit(1);
    }

    codegen_block(prog); 
    
    fprintf(outfile, "/* Generated C code from IML Compiler */\n\n");
    fprintf(outfile, "#include \"runtime.h\"\n");
    fprintf(outfile, "#include <stdio.h>\n");
    fprintf(outfile, "#include <stdlib.h>\n\n");
    
    fprintf(outfile, "int main() {\n");

    fprintf(outfile, "\n    // Variable Declarations (Hoisted from IML script)\n");
    VarEntry *current = var_list;
    while(current) {
        fprintf(outfile, "    %s %s%s;\n", 
            type_to_c_string(current->type), 
            current->name, 
            (current->type == TYPE_IMAGE || current->type == TYPE_STRING) ? " = NULL" : ""); 
        
        VarEntry *temp = current;
        current = current->next;
        free(temp->name);
        free(temp); 
    }
    var_list = NULL; 

    fprintf(outfile, "\n    // Program Logic (Assignments and Expressions)\n");
    if (code_buffer) {
        fprintf(outfile, "%s", code_buffer);
        free(code_buffer);
        code_buffer = NULL;
        buffer_used = 0;
        buffer_size = 0;
    }

    fprintf(outfile, "\n\n    return 0;\n");
    fprintf(outfile, "}\n");
    
    temp_counter = 0;
    
    fclose(outfile);
}