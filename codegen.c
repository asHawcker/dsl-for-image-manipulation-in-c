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
void codegen_expr(Ast *expr); 

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

int is_void_func(const char *fname) {
    return strcmp(fname, "save") == 0 || strcmp(fname, "print") == 0;
}

void codegen_expr(Ast *expr) {
    if (!expr) {
        buffer_printf("NULL");
        return;
    }
    
    switch (expr->type) {
        case AST_INT_LIT: 
            buffer_printf("%d", expr->ival);
            break;
        case AST_FLOAT_LIT: 
            buffer_printf("%lf", expr->fval);
            break;
        case AST_STRING_LIT: 
            buffer_printf("\"%s\"", expr->sval);
            break;
        case AST_NULL_LIT: 
            buffer_printf("NULL");
            break;
        case AST_IDENT: 
            buffer_printf("%s", expr->ident.str);
            break;

        case AST_CALL: {
            const char *c_func_name = map_iml_to_c_func(expr->call.name); 
            buffer_printf("%s(", c_func_name);
            
            for (int i = 0; i < expr->call.nargs; i++) {
                codegen_expr(expr->call.args[i]);
                if (i < expr->call.nargs - 1) buffer_printf(", ");
            }
            buffer_printf(")");
            break;
        }

        case AST_BINOP: {
            buffer_printf("(");
            codegen_expr(expr->binop.left);
            
            const char *op_str;
            switch(expr->binop.op) {
                case PLUS: op_str = "+"; break;
                case MINUS: op_str = "-"; break;
                case MUL: op_str = "*"; break;
                case DIV: op_str = "/"; break;
                case MOD: op_str = "%%"; break;
                case EQ: op_str = "=="; break;
                case NEQ: op_str = "!="; break;
                case GT: op_str = ">"; break;
                case LT: op_str = "<"; break;
                case GE: op_str = ">="; break;
                case LE: op_str = "<="; break;
                default: op_str = "?";
            }
            buffer_printf(" %s ", op_str);
            
            codegen_expr(expr->binop.right);
            buffer_printf(")");
            break;
        }
    
        
        default:
            buffer_printf("/* ERROR: Unknown expression type %d */", expr->type);
    }
}


void codegen_stmt(Ast *stmt) {
    if (!stmt) return;

    int current_indent = 1; 
    write_indent(current_indent);
    
    switch (stmt->type) {
        case AST_DECL: {
            register_var(stmt->decl.name, stmt->decl.type_node->type2);
            buffer_printf("%s = ", stmt->decl.name); 
            codegen_expr(stmt->decl.expr);
            buffer_printf(";");
            break;
        }

        case AST_ASSIGN: {
            register_var(stmt->assign.name, TYPE_IMAGE); 
            buffer_printf("%s = ", stmt->assign.name);
            codegen_expr(stmt->assign.expr);
            buffer_printf(";");
            break;
        }

        case AST_EXPR_STMT: {
            if (stmt->expr_stmt.expr->type == AST_CALL) {
                if (is_void_func(stmt->expr_stmt.expr->call.name)) {
                    codegen_expr(stmt->expr_stmt.expr);
                    buffer_printf(";");
                    break;
                }
            }
            
            codegen_expr(stmt->expr_stmt.expr);
            buffer_printf("; /* result ignored */");
            break;
        }
        
        default:
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
    fprintf(outfile, "\n    // Program Logic\n");
    if (code_buffer) {
        fprintf(outfile, "%s\n", code_buffer);
        free(code_buffer);
        code_buffer = NULL;
    }

    fprintf(outfile, "\n\n    return 0;\n");
    fprintf(outfile, "}\n");
    
    fclose(outfile);
}   