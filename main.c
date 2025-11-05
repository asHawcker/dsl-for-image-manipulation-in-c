#include "ast.h"
#include "codegen.h" 
#include "optimize.h" // <-- NEW: Include the optimizer
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Ast *root;
extern int yyparse();
extern FILE *yyin;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <script.iml>\n", argv[0]);
        return 1;
    }
    const char *input_file = argv[1];
    const char *output_c_file = "generated_code.c";
    const char *output_bin_file = "a.out"; // Changed to a.out
    
    // 1. Parsing (IML Source -> AST)
    yyin = fopen(input_file, "r");
    if (!yyin) {
        perror("fopen");
        return 1;
    }

    if (yyparse() != 0) {
        printf("Parse failed\n");
        fclose(yyin);
        return 1;
    }
    fclose(yyin);

    // 2. OPTIMIZATION (AST -> Optimized AST)
    printf("Optimizing AST...\n");
    root = optimize_ast(root); // Perform the crucial inline and simplification step

    // 3. Code Generation (Optimized AST -> C Source)
    printf("Compiling: Generating clean C code to %s...\n", output_c_file);
    codegen_program(root, output_c_file);

    // 4. C Compilation (C Source + Runtime -> Executable)
    char compile_command[512];
    sprintf(compile_command, "gcc -o %s %s runtime.c -O2 -lm -Wall -I.", 
            output_bin_file, output_c_file);
            
    printf("Compiling: Executing C compiler: %s\n", compile_command);
    if (system(compile_command) != 0) {
        fprintf(stderr, "C compilation failed! Check generated_code.c for errors.\n");
        free_ast(root);
        return 1;
    }
     /*
    // 5. Execution (Executable Run)
    printf("Execution: Running compiled program...\n");
    char execute_command[128];
    sprintf(execute_command, "./%s", output_bin_file);
    if (system(execute_command) != 0) {
        fprintf(stderr, "Execution of %s failed.\n", output_bin_file);
        free_ast(root);
        return 1;
    }
        */
    
    printf("Success! Program finished.\n");
    
    free_ast(root);
    return 0;
}